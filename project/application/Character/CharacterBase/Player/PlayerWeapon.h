#pragma once
#include "PlayerIWeapon.h"
#include "CollisionTypeIdDef.h"
#include "PlayerBullet.h"
#include "PlayerAnimKey.h"
#include "IPlayerSkill.h"
#include "TargetPicker.h"

#include <memory>
#include <vector>
#include <unordered_set>

class Sword;

//======================================================
// PlayerWeapon
//------------------------------------------------------
// プレイヤーの武器・攻撃制御クラス（PlayerIWeapon 実装）。
//
// 役割は「調停」：
//   - ノーマル攻撃のコンボ管理（当たり判定はアニメ進行度0〜1に紐づく）
//   - スキル(IPlayerSkill)の所有・入力ルーティング・排他制御
//     （スキルの中身＝振り付けは各スキルクラスが持つ）
//       Skill   : E キー（回転斬り）
//       Skill2  : V キー（突進乱舞）
//       Ultimate: Q キー（斬奪＝ロックオン＋線合わせ斬撃）
//======================================================
class PlayerWeapon : public PlayerIWeapon{
public:

	PlayerWeapon() {}

	// 弧スキルなどが駆動する剣を注入する（Player が所有。借りるだけ）
	void SetSword(Sword* sword) { sword_ = sword; }

	void Initialize() override;

	void Update() override;

	void UpdateSkills() override;

	void Draw() override;

	// 前景UI描画：アルティメットのロックオン枠・斬撃線をスキルへ委譲する。
	void ForeGroundDraw() override;

	void NormalAttack() override;

	void Skill() override;
	void Skill2() override;
	void Ultimate() override;

	void SetEnemyTargetProvider(IEnemyTargetProvider* provider) override;

	// 攻撃モーション中か（アニメ上書き禁止の判定に使う）
	bool IsAttacking() const { return attacking_; }
	bool HasComboReserve() const { return comboReserve_; }

	// E キーのスキル（回転斬り）が発動中か（剣オーラの演出制御などに使う）
	bool IsESkillActive() const { return eSkill_ && eSkill_->IsActive(); }

	//--- スキルの使用回数（E/V/Q はそれぞれ kSkillMaxCharges_ 回まで）---
	// 残り0になったら発動しない。UI（SkillChargePips）へ残数を出すのにも使う。
	static constexpr int kSkillMaxCharges_ = 3;
	int GetESkillCharges() const { return eCharges_; }
	int GetSkill2Charges() const { return skill2Charges_; }
	int GetUltimateCharges() const { return ultCharges_; }

	// 当たり判定を出してよい区間か（攻撃中 かつ ヒットウィンドウ内）。
	// ウィンドウはアニメ進行度(0〜1)で持つので、再生速度を変えても同期する。
	bool IsHitActive() const;

	// 攻撃の調整値を実機で詰めるための ImGui（Debug ビルドのみ中身がある）。
	// シーンの Debug() から呼ぶ。
	void DebugImGui();

private:

	// いずれかのスキルが発動中か（攻撃やスキルの多重発動を防ぐ排他制御に使う）
	bool IsAnySkillActive() const {
		return (eSkill_ && eSkill_->IsActive())
			|| (skill2_ && skill2_->IsActive())
			|| (ultSkill_ && ultSkill_->IsActive());
	}

private:

	//--------------------------------------------------
	// 攻撃1段ぶんの設定。段ごとに振りの速さと各タイミングを調整できる。
	// 時間[秒]ではなくアニメ進行度(0〜1)で持つので、speed を変えても自動で同期する。
	//--------------------------------------------------
	struct AttackInfo {
		PlayerAnimKey key;   // 再生するアニメ
		float speed;         // 再生速度倍率（1.0=等倍 / >1速い / <1遅い）
		float hitStart;      // 当たり判定ON  （進行度 0〜1）
		float hitEnd;        // 当たり判定OFF （進行度 0〜1）
		float comboAt;       // この進行度を超えたら次段へ繋ぐ
		float cancelAt;      // この進行度を超えたら移動入力で振り終わりをキャンセルできる
	};

	// ノーマル攻撃の段数
	static constexpr int kAttackStageCount_ = 3;

	//--------------------------------------------------
	// ノーマル攻撃のテーブル（調整値）。
	//   ・振りの長さ自体はアニメ（モデル側）で決まるので、ここでは speed を触らない
	//   ・cancelAt は「振り終わってから次の行動へ移れる」タイミング。
	//     ここを過ぎれば移動入力で戻りモーションを打ち切れるので、
	//     アニメを最後まで見せつつ待ち時間が出ない。
	//   ・hitEnd ≦ comboAt / cancelAt になるようにする（当たり判定を出し切ってから次へ）
	// 実機で詰められるよう ImGui（Player Attack ウィンドウ）から編集できる。
	//--------------------------------------------------
	AttackInfo normalAttacks_[kAttackStageCount_] = {
		//  key,                    speed, hitStart, hitEnd, comboAt, cancelAt
		{ PlayerAnimKey::Attack01,   1.3f,    0.25f,  0.45f,   0.60f,    0.55f }, // 速い小振り
		{ PlayerAnimKey::Attack02,   1.15f,   0.30f,  0.55f,   0.65f,    0.62f }, // 少し速い小振り
		{ PlayerAnimKey::Attack03,   0.9f,    0.40f,  0.70f,   0.80f,    0.75f }, // 少しゆっくり大振り
	};

	// 指定した段の攻撃を開始する
	void StartAttack(int index);

	// 攻撃状態を終える（移動・次の入力を受け付けられるようにする）
	void EndAttack();

	//--------------------------------------------------
	// ソフトロック：振り始めに、狙っていそうな敵の方へ向きを合わせる。
	//   ・踏み込み（前進）はしない。向きだけを変える
	//   ・基準方向は「移動入力の方向」。入力が無ければ今の向き
	//     （スティックを倒した方向の敵を斬れる＝対象の切り替えが操作でできる）
	//   ・対象が見つからなければ向きを変えない
	//   ・コンボの段ごとに選び直す（StartAttack から呼ぶ）
	//--------------------------------------------------
	void ApplySoftLock();

	// 移動入力が入っているか（振り終わりのキャンセル判定に使う）
	bool HasMoveInput() const;

private:

	// この進行度に達したら攻撃終了とみなす（ワンショットは終端で1.0に張り付く）
	static constexpr float kAttackEndProgress_ = 0.999f;

	// ---- 攻撃状態 ----
	bool attacking_ = false;       // 攻撃モーション中か
	int  activeAttackIndex_ = 0;   // 再生中の段（0,1,2）

	// ---- コンボ ----
	bool comboReserve_ = false;    // 次の攻撃を予約しているか

	// ---- 借り物（所有しない） ----
	Sword* sword_ = nullptr;       // スキルが駆動する剣

	// ---- スキル ----
	// E キーのスキル1（回転斬り）。IPlayerSkill 越しに扱い、追加時は別クラスを足すだけ。
	std::unique_ptr<IPlayerSkill> eSkill_;

	// V キーのスキル2（突進乱舞）。元はアルティメットだった枠を新スキルへ移設した。
	std::unique_ptr<IPlayerSkill> skill2_;

	// Q キーのアルティメット（斬奪）。実体は UltimateSkill。
	std::unique_ptr<IPlayerSkill> ultSkill_;

	// 攻撃対象の供給元（Scene 所有・参照のみ）。
	// スキルへ橋渡しするほか、突進系スキル（V/Q）を撃ってよい局面かの判定に使う。
	IEnemyTargetProvider* provider_ = nullptr;

	// 各スキルの残り使用回数。発動が成立した時だけ減らす
	int eCharges_ = kSkillMaxCharges_;
	int skill2Charges_ = kSkillMaxCharges_;
	int ultCharges_ = kSkillMaxCharges_;

	// ---- ソフトロック（調整値）----
	// 狙う敵の選び方は targetPicker_ が持つ（射程・角度・重み）。
	TargetPicker targetPicker_;
	bool  softLockEnabled_ = true;        // 切って従来どおりにもできる
	float softLockTurnSeconds_ = 0.08f;   // 目標の向きへ向き切るまでの時間[秒]

	// デバッグ用フラグ
	bool showDebug_ = true;
};
