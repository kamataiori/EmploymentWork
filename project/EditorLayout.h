#pragma once

#ifdef USE_IMGUI

#include <externals/imgui/imgui.h>
#include <string>

/// <summary>
/// UE5風レイアウトを管理するクラス
/// 
/// ドッキング可能な5パネル構成:
///   ┌────────┬──────────────────────┬──────────────┐
///   │        │                      │              │
///   │        │       Viewport       │   Outliner   │
///   │ Actors │                      │              │
///   │ Palette├──────────────────────┼──────────────┤
///   │        │                      │              │
///   │        │   Content Browser    │   Details    │
///   │        │                      │              │
///   └────────┴──────────────────────┴──────────────┘
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
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// フレーム終了処理
	/// MyGame::Update() の末尾で呼ぶ
	/// </summary>
	void EndFrame();

	/// <summary>レイアウトを初期状態にリセット</summary>
	void ResetLayout();

public:
	//------ゲッター/セッター------//

	bool IsEnabled() const { return enabled_; }
	void SetEnabled(bool enable) { enabled_ = enable; }

	/// <summary>
	/// パフォーマンス統計を注入する
	/// MyGame::Update() 内で計測した結果を毎フレーム渡す
	/// </summary>
	void SetPerformanceStats(float fps, float frameTimeMs, float averageFps)
	{
		stat_fps_ = fps;
		stat_frameTimeMs_ = frameTimeMs;
		stat_averageFps_ = averageFps;
	}

private:
	//------ヘルパー関数（パネルごとに分割）------//

	/// <summary>メインメニューバー</summary>
	void DrawMenuBar();

	/// <summary>メニューバー内: Scene メニュー</summary>
	void DrawSceneMenu();

	/// <summary>DockSpaceを構築（初回のみレイアウトを設定）</summary>
	void BuildDockSpace();

	/// <summary>Viewport (中央上: 実際のゲーム画面を表示)</summary>
	void DrawViewportPanel();

	/// <summary>Actors Palette (左: アクタを配置するパネル)</summary>
	void DrawActorsPalettePanel();

	/// <summary>Content Browser (中央下: アセット一覧)</summary>
	void DrawContentBrowserPanel();

	/// <summary>Outliner (右上: シーン内のオブジェクト一覧)</summary>
	void DrawOutlinerPanel();

	/// <summary>Details / Inspector (右下: 選択オブジェクトの詳細)</summary>
	void DrawDetailsPanel();

	/// <summary>Viewport上に重ねるFPSオーバーレイ (UE5のStat FPS風)</summary>
	void DrawStatFPSOverlay();

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
	bool showActorsPalette_ = true;
	bool showContentBrowser_ = true;
	bool showOutliner_ = true;
	bool showDetails_ = true;

	// レイアウトリセット要求フラグ
	bool requestResetLayout_ = false;

	// シーン切り替え予約
	std::string requestedSceneName_;

	// ---- Stat FPS オーバーレイ用 ----
	// MyGame から注入されるパフォーマンス統計
	float stat_fps_ = 0.0f;
	float stat_frameTimeMs_ = 0.0f;
	float stat_averageFps_ = 0.0f;
	// Viewport上にStat FPSを表示するかどうか
	bool  showStatFPS_ = true;

	// 各パネル名（ウィンドウ識別用）
	static constexpr const char* kDockSpaceName = "EditorDockSpace";
	static constexpr const char* kViewportName = "Viewport";
	static constexpr const char* kActorsPaletteName = "Actors";
	static constexpr const char* kContentBrowserName = "Content Browser";
	static constexpr const char* kOutlinerName = "Outliner";
	static constexpr const char* kDetailsName = "Details";
};

#else  // USE_IMGUI が無効なとき（Release）

/// <summary>Release ビルド用のスタブ</summary>
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
	void SetPerformanceStats(float, float, float) {}
};

#endif // USE_IMGUI