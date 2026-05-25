#pragma once
#include <windows.h>

/// <summary>
/// マウスカーソルの「表示/非表示」と「ウィンドウ内への閉じ込め」を
/// シーン毎に切り替えるためのサービス。
///
/// 設計:
///   - シングルトンではなく、Framework が unique_ptr で所有する。
///   - シーン側は ShouldShowCursor / ShouldConfineCursor で要求を宣言するだけ。
///   - SceneManager と MyGame には Framework からポインタを注入する。
///   - フォーカス検出は GetForegroundWindow を Update() でポーリングするので、
///     WindowProc 等の static コードから触る必要は無い。
///   - エディタ停止中 (USE_IMGUI かつ Stop) はカーソルを強制解放する。
///   - Update() で毎フレーム、ウィンドウ位置/サイズが変わってもクリップ矩形を最新に保つ。
/// </summary>
class CursorService
{
public:
	CursorService() = default;
	~CursorService();

	CursorService(const CursorService&) = delete;
	CursorService& operator=(const CursorService&) = delete;

	void Initialize(HWND hwnd);

	/// <summary>
	/// シーンが要求するカーソル設定を反映する。
	/// (SceneManager のシーン切替直後に1回呼ぶ)
	/// </summary>
	void ApplySceneRequest(bool visible, bool confined);

	/// <summary>
	/// 毎フレーム呼ぶ。
	/// - フォーカス変化を検出 (GetForegroundWindow)
	/// - ウィンドウ移動/リサイズに追従してクリップ矩形を更新
	/// </summary>
	void Update();

	/// <summary>
	/// エディタ停止中などで「シーン設定を無視してカーソルを強制解放する」モード。
	/// MyGame 側で Play/Stop の状態に合わせて切り替える想定。
	/// </summary>
	void SetEditorOverride(bool forceFreeCursor);

	bool IsVisible() const { return effectiveVisible_; }
	bool IsConfined() const { return effectiveConfined_; }

private:
	void Reapply();
	void ApplyVisible(bool visible);
	void ApplyConfined(bool confined);

	HWND hwnd_ = nullptr;

	// シーンが希望している設定 (要求値)
	bool requestedVisible_ = true;
	bool requestedConfined_ = false;

	// 実際にOSに反映している設定 (実効値)
	bool effectiveVisible_ = true;
	bool effectiveConfined_ = false;

	// フォーカス状態 (Update内で GetForegroundWindow から導出)
	bool hasFocus_ = true;

	// エディタによる強制解放
	bool editorOverride_ = false;
};
