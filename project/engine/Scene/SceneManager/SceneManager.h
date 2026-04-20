#pragma once
#include "BaseScene.h"
#include "TitleScene.h"
#include "AbstractSceneFactory.h"
#include <string>

struct TransitionRequest;
class SceneTransitionService;
class PlayModeState;

class SceneManager
{
public:
	static SceneManager* instance;

	// インスタンスを取得するシングルトンメソッド
	static SceneManager* GetInstance();

	// プライベートコンストラクタ
	SceneManager() = default;

	// コピーコンストラクタおよび代入演算子を削除
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

public:
	//------メンバ関数------

	// デストラクタ
	~SceneManager();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();


public:
	// 次シーン予約
	void SetNextScene(BaseScene* nextScene) { nextScene_ = nextScene; }

	// シーンファクトリーのSetter
	void SetSceneFactory(AbstractSceneFactory* factory)
	{
		sceneFactory_ = factory;
	}

	// シーンファクトリーのGetter (EditorLayout等のUIから参照する用)
	AbstractSceneFactory* GetSceneFactory() const { return sceneFactory_; }

	// 現在のシーンを取得
	BaseScene* GetCurrentScene() const { return scene_; }

	/// <summary>
	/// 現在アクティブなシーン名を取得
	/// (ChangeScene で指定された名前を覚えている)
	/// </summary>
	const std::string& GetCurrentSceneName() const { return currentSceneName_; }

	/// <summary>
	/// 次シーン予約
	/// </summary>
	/// <param name="sceneName"></param>
	void ChangeScene(const std::string& sceneName);

	/// <summary>
	/// 現在のシーンを同じ名前で作り直す
	/// (UE5のPlayボタンによるシーンリセット相当)
	/// </summary>
	void ReloadCurrentScene();

	// -----Scene切り替え演出----- //

	void SetTransitionService(SceneTransitionService* service) { transitionService_ = service; }
	void RequestChangeScene(const std::string& nextSceneName, const TransitionRequest& req);
	bool IsTransitioning() const;

	// -----Play/Stop制御----- //

	/// <summary>
	/// Play/Stop 状態を監視対象として設定する
	/// (セットされていれば、停止中は scene_->Update() がスキップされる。
	///  ただしシーン切替直後の1フレームだけは強制的にUpdateを呼び、
	///  描画可能な状態を保証する)
	/// 所有権は持たない
	/// </summary>
	void SetPlayModeState(PlayModeState* state) { playModeState_ = state; }

private:

	// 今のシーン (実行中シーン)
	BaseScene* scene_ = nullptr;

	// 次のシーン
	BaseScene* nextScene_ = nullptr;

	// 最後に ChangeScene に渡された名前 (現在アクティブなシーン名)
	std::string currentSceneName_;

	// シーンファクトリー (借りてくる)
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// 遷移実行はここに委譲する(所有はしない)
	SceneTransitionService* transitionService_ = nullptr;

	// Play/Stop 状態 (外部から注入。nullなら常に再生扱い)
	PlayModeState* playModeState_ = nullptr;
};