#pragma once
#include "SceneInputBase.h"

/// <summary>
/// タイトルシーンの入力。
///
/// シーンは「どのキーか / パッドか」を一切知らない。
/// ここが唯一の割り当て表 (下の k○○Key_ / k○○Button_) で、
/// キーコンフィグを入れるときもこのクラスだけを触れば済む。
///
/// moveX / moveY は -1.0〜1.0 の軸として持つ。
/// キーボードは倒し切り (±1)、パッドは左スティックの傾きがそのまま入るので、
/// 「メニューを選ぶ」ようなデジタル用途にも、
/// 「カメラを少し動かす」ようなアナログ用途にも同じ値が使える。
/// (タイトルにメニューを追加するときは moveY の符号を見て選択を動かす)
///
/// ポーズのような“シーン側の入力”は、同じ作りで GameSceneInput に pause_ を足す。
/// </summary>
class TitleSceneInput : public SceneInputBase
{
public:
	explicit TitleSceneInput(Input* input) : SceneInputBase(input) {}

	/// <summary>入力のスナップショットを取る (シーンの Update 先頭で 1 回)</summary>
	void Update() override;

	///========================
	// 取得 (シーンはこれだけを見る)
	///========================

	/// 横軸 (-1:左 〜 +1:右)
	float GetMoveX() const { return moveX_; }
	/// 縦軸 (-1:下 〜 +1:上)
	float GetMoveY() const { return moveY_; }

	/// 決定を押した瞬間 (ゲーム開始)
	bool IsDecideTriggered() const { return decide_; }
	/// キャンセルを押した瞬間 (メニューを付けたときの「戻る」用)
	bool IsCancelTriggered() const { return cancel_; }

private:
	///========================
	// 割り当て表
	///========================

	// 移動 (WASD)
	static constexpr BYTE kMoveLeftKey_ = DIK_A;
	static constexpr BYTE kMoveRightKey_ = DIK_D;
	static constexpr BYTE kMoveDownKey_ = DIK_S;
	static constexpr BYTE kMoveUpKey_ = DIK_W;

	// 移動 (十字キー。WASD と同じ軸に足して両対応にする)
	static constexpr BYTE kMoveLeftArrowKey_ = DIK_LEFT;
	static constexpr BYTE kMoveRightArrowKey_ = DIK_RIGHT;
	static constexpr BYTE kMoveDownArrowKey_ = DIK_DOWN;
	static constexpr BYTE kMoveUpArrowKey_ = DIK_UP;

	// 決定 / キャンセル
	static constexpr BYTE kDecideKey_ = DIK_SPACE;
	static constexpr PadButton kDecideButton_ = PadButton::A;
	static constexpr BYTE kCancelKey_ = DIK_ESCAPE;
	static constexpr PadButton kCancelButton_ = PadButton::B;

	///========================
	// 今フレームの入力
	///========================

	float moveX_ = kAxisNeutral_;
	float moveY_ = kAxisNeutral_;
	bool decide_ = false;
	bool cancel_ = false;
};
