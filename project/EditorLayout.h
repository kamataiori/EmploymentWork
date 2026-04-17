#pragma once

#ifdef USE_IMGUI

#include <externals/imgui/imgui.h>
#include <string>

/// <summary>
/// UE5風レイアウトを管理するクラス
/// 
/// ドッキング可能な5パネル構成:
///   ┌──────────┬────────────────────┬─────────────┐
///   │          │                    │             │
///   │ Outliner │      Viewport      │   Details   │
///   │          │                    │             │
///   ├──────────┤                    ├─────────────┤
///   │ Content  │                    │   World     │
///   │ Browser  │                    │  Settings   │
///   └──────────┴────────────────────┴─────────────┘
/// </summary>
class EditorLayout
{
public:
	//------メンバ関数------//

	/// <summary>初期化</summary>
	void Initialize();

	/// <summary>終了</summary>
	void Finalize();

	/// <summary>
	/// フレーム開始処理
	/// MyGame::Update() の冒頭で呼ぶ
	/// ImGuiのDockSpace/MenuBar/各パネルを描画する
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// フレーム終了処理
	/// MyGame::Update() の末尾で呼ぶ
	/// （シーン切り替え予約の実行などはここで行う）
	/// </summary>
	void EndFrame();

	/// <summary>レイアウトを初期状態にリセット</summary>
	void ResetLayout();

public:
	//------ゲッター/セッター------//

	/// <summary>エディタ全体の表示/非表示</summary>
	bool IsEnabled() const { return enabled_; }
	void SetEnabled(bool enable) { enabled_ = enable; }

private:
	//------ヘルパー関数（パネルごとに分割）------//

	/// <summary>メインメニューバー</summary>
	void DrawMenuBar();

	/// <summary>メニューバー内: Scene メニュー</summary>
	void DrawSceneMenu();

	/// <summary>DockSpaceを構築（初回のみレイアウトを設定）</summary>
	void BuildDockSpace();

	/// <summary>Viewport (中央: 実際のゲーム画面を表示)</summary>
	void DrawViewportPanel();

	/// <summary>Outliner (左上: シーン内のオブジェクト一覧)</summary>
	void DrawOutlinerPanel();

	/// <summary>Content Browser (左下: アセット一覧)</summary>
	void DrawContentBrowserPanel();

	/// <summary>Details / Inspector (右上: 選択オブジェクトの詳細)</summary>
	void DrawDetailsPanel();

	/// <summary>World Settings (右下: ワールド全体の設定)</summary>
	void DrawWorldSettingsPanel();

	/// <summary>カスタムスタイルの適用</summary>
	void ApplyStyle();

private:
	//------メンバ変数------//

	// エディタUI全体の表示フラグ
	bool enabled_ = true;

	// DockSpaceの初期レイアウト構築済みフラグ
	bool dockLayoutBuilt_ = false;

	// 各パネルの表示/非表示
	bool showViewport_ = true;
	bool showOutliner_ = true;
	bool showContentBrowser_ = true;
	bool showDetails_ = true;
	bool showWorldSettings_ = true;

	// レイアウトリセット要求フラグ
	bool requestResetLayout_ = false;

	// ---- シーン切り替え用 ----
	// シーン切り替えは BeginFrame の途中で SceneManager に発行すると
	// 同フレーム内でシーンが作り変わって危険なので、予約しておいて
	// EndFrame で実行する方針にする
	std::string requestedSceneName_;  // 空なら予約なし

	// 各パネル名（ウィンドウ識別用）
	static constexpr const char* kDockSpaceName = "EditorDockSpace";
	static constexpr const char* kViewportName = "Viewport";
	static constexpr const char* kOutlinerName = "Outliner";
	static constexpr const char* kContentBrowserName = "Content Browser";
	static constexpr const char* kDetailsName = "Details";
	static constexpr const char* kWorldSettingsName = "World Settings";
};

#else  // USE_IMGUI が無効なとき（Release）

/// <summary>
/// Release ビルド用のスタブ
/// </summary>
class EditorLayout
{
public:
	void Initialize() {}
	void Finalize() {}
	void BeginFrame() {}
	void EndFrame() {}
	void ResetLayout() {}
	bool IsEnabled() const { return false; }
	void SetEnabled(bool) {}
};

#endif // USE_IMGUI