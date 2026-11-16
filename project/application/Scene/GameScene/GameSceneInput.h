#pragma once
#include "SceneInputBase.h"
#include "MenuInput.h"
#include "IPlayerInput.h"
#include "IAimInput.h"
#include "ICameraInput.h"

/// <summary>
/// ゲームプレイシーンの入力。
///
/// 継承は3つだが、役割はそれぞれ違う（深い継承ではなく「土台1つ＋読む側ごとの口」）:
///   - SceneInputBase : 「InputReader を持ち、毎フレーム Update で取る」という共通の作り
///   - IPlayerInput   : 「プレイヤーが読む入力はこれ」という口
///   - ICameraInput   : 「追従カメラが読む入力はこれ」という口
///   - IAimInput      : 「アルティメットの照準が読む入力はこれ」という口
///   口を読む側ごとに分けているのは、カメラが移動や攻撃を知る必要が無く、
///   プレイヤーが視点感度を知る必要も無いため（必要な分だけを見せる）。
/// 操作の中身は ButtonAction / AxisInput / LookAxis を必要な数だけ持つ（コンポーネント）。
///
/// 入力の担当分け:
///   ・移動 / 攻撃 / スキル → プレイヤーが IPlayerInput 経由で読む
///   ・視点（周回角・見下ろし角）→ FollowCamera が ICameraInput 経由で読む
///   ・アルティメットの照準（斬撃線）→ UltimateSkill が IAimInput 経由で読む
///   ・ポーズ / デバッグ    → シーンが直接読む（プレイヤーにもカメラにも関係しない操作）
///   ・ポーズ画面のメニュー操作 → MenuInput を1つ持って UI 要素へ貸し出す
///     （メニュー操作はタイトルと同じなので、継承ではなく共通クラスを持つ形にしている）
/// </summary>
class GameSceneInput : public SceneInputBase, public IPlayerInput, public IAimInput, public ICameraInput
{
public:
	explicit GameSceneInput(Input* input);

	/// <summary>今フレームの入力を取る（シーンの Update 先頭で1回）</summary>
	void Update() override;

	///========================
	// シーンが見る入力
	///========================

	/// ポーズの開閉を押した瞬間（ESC / パッドStart）
	bool IsPauseTriggered() const { return pause_.IsTriggered(); }

	/// デバッグ：タイトルへ戻るを押した瞬間
	bool IsDebugReturnToTitleTriggered() const { return debugReturnToTitle_.IsTriggered(); }

	/// ポーズ画面など UI 要素へ貸し出すメニュー操作（所有はこのクラス）
	const IMenuInput* GetMenuInput() const { return &menu_; }

	/// ジャンプ／ダッシュを押した瞬間（チュートリアルの「操作できたか」判定用）
	bool IsJumpTriggered() const { return jump_.IsTriggered(); }
	bool IsDashTriggered() const { return dash_.IsTriggered(); }

	///========================
	// プレイヤーが見る入力（IPlayerInput）
	///========================

	float GetMoveX() const override { return moveX_.GetValue(); }
	float GetMoveY() const override { return moveY_.GetValue(); }

	bool IsJumpHeld() const override { return jump_.IsPressed(); }
	bool IsDashHeld() const override { return dash_.IsPressed(); }
	bool IsAttackHeld() const override { return attack_.IsPressed(); }

	bool IsAttackTriggered() const override { return attack_.IsTriggered(); }
	bool IsSkillTriggered() const override { return skill_.IsTriggered(); }
	bool IsSkill2Triggered() const override { return skill2_.IsTriggered(); }
	bool IsUltimateTriggered() const override { return ultimate_.IsTriggered(); }

	///========================
	// アルティメットの照準が見る入力（IAimInput）
	///========================

	float MoveAimX(float currentX, float deltaTime) const override {
		return aimX_.Move(Reader(), currentX, deltaTime);
	}
	float MoveAimY(float currentY, float deltaTime) const override {
		return aimY_.Move(Reader(), currentY, deltaTime);
	}

	float GetAimCenterX() const override { return aimX_.GetCenter(); }
	float GetAimCenterY() const override { return aimY_.GetCenter(); }

	// 斬撃の確定は通常攻撃と、カメラの左右入れ替えはダッシュと同じボタンを共有する。
	// 「斬る」「身をかわす」操作をそのまま流用する意図の共有なので、
	// 割り当てを2つに分けず、ここで言い換えるだけにしてある。
	bool IsAimConfirmTriggered() const override { return attack_.IsTriggered(); }
	bool IsAimSideSwapTriggered() const override { return dash_.IsTriggered(); }

	///========================
	// 追従カメラが見る入力（ICameraInput）
	///========================

	LookAxisValue GetYawLook() const override { return yawLook_.GetValue(); }
	LookAxisValue GetPitchLook() const override { return pitchLook_.GetValue(); }

private:
	// 移動（十字キーは FollowCamera の旋回に使われているので副割り当てには入れない）
	AxisInput moveX_;
	AxisInput moveY_;

	// プレイヤーの操作
	ButtonAction jump_;
	ButtonAction dash_;
	ButtonAction attack_;
	ButtonAction skill_;
	ButtonAction skill2_;
	ButtonAction ultimate_;

	// 視点（周回角・見下ろし角）
	LookAxis yawLook_;
	LookAxis pitchLook_;

	// 照準（斬撃線）。位置はスキル側が持つので、ここは混ぜ方だけを持つ（状態なし）
	AimAxis aimX_;
	AimAxis aimY_;

	// ポーズ画面のメニュー操作（タイトルと共通の割り当て表を使う）
	MenuInput menu_;

	// シーンの操作
	ButtonAction pause_;
	ButtonAction debugReturnToTitle_;
};
