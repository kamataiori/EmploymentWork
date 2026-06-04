#include "CursorService.h"

CursorService::~CursorService()
{
	// 終了時はカーソルが必ず表示・解放状態になるように戻す。
	if (!effectiveVisible_) {
		ApplyVisible(true);
	}
	if (effectiveConfined_) {
		ApplyConfined(false);
	}
}

void CursorService::Initialize(HWND hwnd)
{
	hwnd_ = hwnd;

	// 初期はエンジン外と同じ「表示+解放」状態
	requestedVisible_ = true;
	requestedConfined_ = false;
	effectiveVisible_ = true;
	effectiveConfined_ = false;
	hasFocus_ = (GetForegroundWindow() == hwnd_);
	editorOverride_ = false;
}

void CursorService::ApplySceneRequest(bool visible, bool confined)
{
	requestedVisible_ = visible;
	requestedConfined_ = confined;
	Reapply();
}

void CursorService::Update()
{
	// フォーカス状態をポーリングで検出 (WindowProc に依存しないため)
	const bool focusedNow = (hwnd_ != nullptr) && (GetForegroundWindow() == hwnd_);
	if (focusedNow != hasFocus_) {
		hasFocus_ = focusedNow;
		Reapply();
	}

	// ウィンドウが移動/リサイズされた場合に備えて、毎フレーム閉じ込め矩形を取り直す。
	if (effectiveConfined_) {
		ApplyConfined(true);
	}
}

void CursorService::SetEditorOverride(bool forceFreeCursor)
{
	if (editorOverride_ == forceFreeCursor) {
		return;
	}
	editorOverride_ = forceFreeCursor;
	Reapply();
}

void CursorService::Reapply()
{
	// 強制解放条件: フォーカス無し OR エディタ停止中
	const bool forceFree = (!hasFocus_) || editorOverride_;

	const bool wantVisible = forceFree ? true : requestedVisible_;
	const bool wantConfined = forceFree ? false : requestedConfined_;

	if (wantVisible != effectiveVisible_) {
		ApplyVisible(wantVisible);
	}
	if (wantConfined != effectiveConfined_) {
		ApplyConfined(wantConfined);
	}
}

void CursorService::ApplyVisible(bool visible)
{
	// ShowCursor は内部カウンタ式 (>=0 で表示, <0 で非表示)。
	// 何度呼んでも目的の状態に収束するようにループで揃える。
	if (visible) {
		int count = ShowCursor(TRUE);
		while (count < 0) {
			count = ShowCursor(TRUE);
		}
	} else {
		int count = ShowCursor(FALSE);
		while (count >= 0) {
			count = ShowCursor(FALSE);
		}
	}
	effectiveVisible_ = visible;
}

void CursorService::ApplyConfined(bool confined)
{
	if (confined && hwnd_) {
		RECT rc{};
		if (GetClientRect(hwnd_, &rc)) {
			POINT lt{ rc.left,  rc.top };
			POINT rb{ rc.right, rc.bottom };
			ClientToScreen(hwnd_, &lt);
			ClientToScreen(hwnd_, &rb);
			RECT screenRc{ lt.x, lt.y, rb.x, rb.y };
			ClipCursor(&screenRc);
		}
	} else {
		ClipCursor(nullptr);
	}
	effectiveConfined_ = confined;
}
