#pragma once
#include "PlayerIWeapon.h"
#include "CollisionTypeIdDef.h"
#include "PlayerBullet.h"
#include "PlayerAnimKey.h"

#include <memory>
#include <vector>
#include <unordered_set>

//======================================================
// PlayerWeapon
//------------------------------------------------------
// プレイヤーの武器・攻撃制御クラス（PlayerIWeapon 実装）
// ノーマル攻撃のコンボ管理・スキル入力などを担当する。
//
// 当たり判定とコンボ判定は「アニメ進行度(0〜1)」に紐づける。
// 再生速度を変えても進行度は同じ%で進むため、当たり判定が自動で同期する。
//======================================================
class PlayerWeapon : public PlayerIWeapon{
public:

	PlayerWeapon() {}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void NormalAttack() override;

	void Skill() override;
	void Ultimate() override;

	// 攻撃モーション中か（アニメ上書き禁止の判定に使う）
	bool IsAttacking() const { return attacking_; }
	bool HasComboReserve() const { return comboReserve_; }

	// 当たり判定を出してよい区間か（攻撃中 かつ ヒットウィンドウ内）。
	// ウィンドウはアニメ進行度(0〜1)で持つので、再生速度を変えても同期する。
	bool IsHitActive() const;

	// ===== スキル「回転斬り」 =====
	// 発動中か（剣の弧モードと連動。Player が Sword の駆動に使う）
	bool IsSkillActive() const { return skillActive_; }
	// スキルの進行度 0〜1（剣の弧の位置計算に使う）
	float GetSkillProgress() const;

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

	// スキル「回転斬り」を開始する
	void StartSkill();

private:

	// この進行度に達したら攻撃終了とみなす（ワンショットは終端で1.0に張り付く）
	static constexpr float kAttackEndProgress_ = 0.999f;

	// ---- スキル「回転斬り」設定 ----
	// スキルの長さ[秒]。この間ずっと剣の弧＋当たり判定が続く。
	static constexpr float kSkillDuration_ = 1.1f;
	// スキル中に再生する Attack02 の速度倍率（弧の長さに合わせて調整）
	static constexpr float kSkillAnimSpeed_ = 1.0f;
	// スキルのアニメ優先度（ノーマル攻撃=10 より高くして上書きされないようにする）
	static constexpr int   kSkillAnimaPriority_ = 20;

	// ---- 攻撃状態 ----
	bool attacking_ = false;       // 攻撃モーション中か
	int  activeAttackIndex_ = 0;   // 再生中の段（0,1,2）

	// ---- コンボ ----
	bool comboReserve_ = false;    // 次の攻撃を予約しているか

	// ---- スキル「回転斬り」状態 ----
	bool  skillActive_ = false;    // スキル発動中か
	float skillElapsed_ = 0.0f;    // 発動からの経過時間[秒]

	// 入力エッジ検出用
	bool IsSkill_ = false;

	// デバッグ用フラグ
	bool showDebug_ = true;
};
