#include "MenuInput.h"

namespace {

	//======================================================
	// 割り当て表（メニュー操作）
	//   タイトル／メニューシーン／ポーズ画面が共通で使う。
	//======================================================

	constexpr AxisBinding kMoveXBinding{
		.negativeKey = DIK_A,
		.positiveKey = DIK_D,
		.negativeAltKey = DIK_LEFT,
		.positiveAltKey = DIK_RIGHT,
		.stick = StickAxis::LeftX,
	};

	constexpr AxisBinding kMoveYBinding{
		.negativeKey = DIK_S,
		.positiveKey = DIK_W,
		.negativeAltKey = DIK_DOWN,
		.positiveAltKey = DIK_UP,
		.stick = StickAxis::LeftY,
	};

	// 選択を1つ送る：W/↑・S/↓ ／ パッドの十字
	constexpr ButtonBinding kUpBinding{
		.key = DIK_W,
		.altKey = DIK_UP,
		.padButton = PadButton::Up,
	};

	constexpr ButtonBinding kDownBinding{
		.key = DIK_S,
		.altKey = DIK_DOWN,
		.padButton = PadButton::Down,
	};

	// 決定：SPACE / パッドA（タイトルの操作ヒント画像と同じ割り当て）
	constexpr ButtonBinding kDecideBinding{
		.key = DIK_SPACE,
		.padButton = PadButton::A,
	};

	// キャンセル：ESC / パッドB
	constexpr ButtonBinding kCancelBinding{
		.key = DIK_ESCAPE,
		.padButton = PadButton::B,
	};

	// ポインタのクリック：左クリック
	constexpr ButtonBinding kPointerClickBinding{
		.mouseButton = MouseButton::Left,
	};

} // namespace

MenuInput::MenuInput(Input* input)
	: SceneInputBase(input)
	, moveX_(kMoveXBinding)
	, moveY_(kMoveYBinding)
	, up_(kUpBinding)
	, down_(kDownBinding)
	, decide_(kDecideBinding)
	, cancel_(kCancelBinding)
	, pointerClick_(kPointerClickBinding)
{
}

void MenuInput::Update()
{
	moveX_.Update(Reader());
	moveY_.Update(Reader());
	up_.Update(Reader());
	down_.Update(Reader());

	decide_.Update(Reader());
	cancel_.Update(Reader());

	pointerClick_.Update(Reader());
	pointerX_ = Reader().GetMousePosition(MouseAxis::X);
	pointerY_ = Reader().GetMousePosition(MouseAxis::Y);
}
