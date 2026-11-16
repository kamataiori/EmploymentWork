#include "TitleSceneInput.h"

void TitleSceneInput::Update()
{
	// 移動軸：WASD ＋ 十字キー ＋ 左スティックを 1 本の軸にまとめる。
	// (ReadAxisX / ReadAxisY がスティックを含むので、十字キー分はキーだけ足す)
	moveX_ = ClampAxis(
		ReadAxisX(kMoveLeftKey_, kMoveRightKey_)
		+ ReadKeyAxis(kMoveLeftArrowKey_, kMoveRightArrowKey_));

	moveY_ = ClampAxis(
		ReadAxisY(kMoveDownKey_, kMoveUpKey_)
		+ ReadKeyAxis(kMoveDownArrowKey_, kMoveUpArrowKey_));

	// 決定 / キャンセル：キーでもパッドでも同じ意味になる
	decide_ = TriggerAny(kDecideKey_, kDecideButton_);
	cancel_ = TriggerAny(kCancelKey_, kCancelButton_);
}
