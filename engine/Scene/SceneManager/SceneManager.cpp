#include "SceneManager.h"

SceneManager* SceneManager::instance = nullptr;

SceneManager* SceneManager::GetInstance()
{
	if (!instance) {
		instance = new SceneManager();
	}
	return instance;
}

void SceneManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

SceneManager::~SceneManager()
{
	// 最期のシーンの終了と解放
	scene_->Finalize();
	delete scene_;
}

void SceneManager::Update()
{
	// TODO:シーン切り替え機構

	// 次のシーンの予約があるなら
	if (nextScene_) {
		// 旧シーンの終了
		if (scene_) {
			scene_->Finalize();
			delete scene_;
		}

		// シーン切り替え
		scene_ = nextScene_;
		nextScene_ = nullptr;

		// シーンマネージャーをセット
		scene_->SetSceneManager(this);

		// 次シーンを初期化する
		scene_->Initialize();
	}

	// 実行中シーンを更新する
	scene_->Update();
}

void SceneManager::Draw()
{
	scene_->BackGroundDraw();
	scene_->Draw();
	scene_->ForeGroundDraw();
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 引数で受け取った次シーン名sceneNameを
	// ファクトリーに渡してシーンを生成させ(CreateScene)、
	// 生成されたシーンを次シーンとしてセットする

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}
