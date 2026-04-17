#pragma once
#include "BaseScene.h"
#include "TitleScene.h"
#include "AbstractSceneFactory.h"
#include <string>

struct TransitionRequest;
class SceneTransitionService;

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

	// シーンファクトリーのGetter（EditorLayout等のUIから参照する用）
	AbstractSceneFactory* GetSceneFactory() const { return sceneFactory_; }

	// 現在のシーンを取得
	BaseScene* GetCurrentScene() const { return scene_; }

	/// <summary>
	/// 次シーン予約
	/// </summary>
	/// <param name="sceneName"></param>
	void ChangeScene(const std::string& sceneName);

	// -----Scene切り替え演出----- // 

	// 遷移要求の窓口（Sceneから呼ぶのは基本これ）
	// SceneManagerは演出の生成を知らず、Serviceに委譲する
	void SetTransitionService(SceneTransitionService* service) { transitionService_ = service; }

	void RequestChangeScene(const std::string& nextSceneName, const TransitionRequest& req);

	bool IsTransitioning() const;

private:

	// 今のシーン (実行中シーン)
	BaseScene* scene_ = nullptr;

	// 次のシーン
	BaseScene* nextScene_ = nullptr;

	// シーンファクトリー (借りてくる)
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// 遷移実行はここに委譲する（所有はしない）
	SceneTransitionService* transitionService_ = nullptr;
};

