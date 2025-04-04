#pragma once
#include "BaseScene.h"
#include "TitleScene.h"
#include "AbstractSceneFactory.h"

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

	/// <summary>
	/// 次シーン予約
	/// </summary>
	/// <param name="sceneName"></param>
	void ChangeScene(const std::string& sceneName);

private:

	// 今のシーン (実行中シーン)
	BaseScene* scene_ = nullptr;

	// 次のシーン
	BaseScene* nextScene_ = nullptr;

	// シーンファクトリー (借りてくる)
	AbstractSceneFactory* sceneFactory_ = nullptr;
};

