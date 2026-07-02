#pragma once
#include "PlayerIWeapon.h"
#include "CollisionTypeIdDef.h"
#include "PlayerBullet.h"
#include "PlayerAnimKey.h"
#include "IPlayerSkill.h"

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

	// 当たり判定を出してよい区間か（攻撃中 かつ ヒットウィンドウ内）。
	// ウィンドウはアニメ進行度(0〜1)で持つので、再生速度を変えても同期する。
	bool IsHitActive() const;

private:

	// いずれかのスキルが発動中か（攻撃やスキルの多重発動を防ぐ排他制御に使う）
	bool IsAnySkillActive() const {
		return (eSkill_ && eSkill_->IsActive())
			|| (skill2_ && skill2_->IsActive())
			|| (ultSkill_ && ultSkill_->IsActive());
	}

private:

	//--------------------------------------------------
	// 攻撃1段ぶんの設定。
	// speed/hitStart/hitEnd/comboAt を段ごとに調整できる。
	//--------------------------------------------------
	struct AttackInfo {
		PlayerAnimKey key;   // 再生するアニメ
		float speed;         // 再生速度倍率（1.0=等倍 / >1速い / <1遅い）
		float hitStart;      // 当たり判定ON  （進行度 0〜1）
		float hitEnd;        // 当たり判定OFF （進行度 0〜1）
		float comboAt;       // この進行度を超えたら次段へ繋ぐ
	};

	// ノーマル攻撃3段ぶんのテーブル。状況に合わせてここを調整する。
	static const AttackInfo kNormalAttacks_[3];

	// 指定した段の攻撃を開始する
	void StartAttack(int index);

private:

	// この進行度に達したら攻撃終了とみなす（ワンショットは終端で1.0に張り付く）
	static constexpr float kAttackEndProgress_ = 0.999f;

	// ---- 攻撃状態 ----
	bool attacking_ = false;       // 攻撃モーション中か
	int  activeAttackIndex_ = 0;   // 再生中の段（0,1,2）

	// ---- コンボ ----
	bool comboReserve_ = false;    // 次の攻撃を予約しているか

	// 入力エッジ検出用（E キーの押した瞬間を取る）
	bool IsSkill_ = false;
	// 入力エッジ検出用（V キーの押した瞬間を取る）
	bool isSkill2_ = false;
	// 入力エッジ検出用（Q キーの押した瞬間を取る）
	bool isUltimate_ = false;

	// ---- 借り物（所有しない） ----
	Sword* sword_ = nullptr;       // スキルが駆動する剣

	// ---- スキル ----
	// E キーのスキル1（回転斬り）。IPlayerSkill 越しに扱い、追加時は別クラスを足すだけ。
	std::unique_ptr<IPlayerSkill> eSkill_;

	// V キーのスキル2（突進乱舞）。元はアルティメットだった枠を新スキルへ移設した。
	std::unique_ptr<IPlayerSkill> skill2_;

	// Q キーのアルティメット。中身（IPlayerSkill 実装）は今後追加する予定で、現状は未割り当て。
	std::unique_ptr<IPlayerSkill> ultSkill_;

	// デバッグ用フラグ
	bool showDebug_ = true;
};
