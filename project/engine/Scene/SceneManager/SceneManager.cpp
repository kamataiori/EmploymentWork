#include "SceneManager.h"
#include <PostEffectManager.h>
#include "engine/Scene/ChangeEffect/SceneTransitionService.h"
#include "engine/Scene/ChangeEffect/SceneTransitionTypes.h"
#include "PlayModeState.h"

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
	if (scene_) {
		scene_->Finalize();
		delete scene_;
		scene_ = nullptr;
	}
}

void SceneManager::Update()
{
	// --- (1) 次のシーンの予約があればシーン切り替え ---
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

		// sceneの最初は基本的にPostEffectをNormalに
		PostEffectManager::GetInstance()->SetType(PostEffectType::Normal);

		// 次シーンを初期化する
		scene_->Initialize();

		// ★ 切替直後に UpdateCamera と LateUpdate を強制実行する
		//    これにより、停止中(Stop)でも初期フレームから
		//    カメラ行列が最新の状態になる → スカイボックス等が正しく描画される
		//
		//    Update (ゲームロジック) は呼ばない
		//    = キャラが進んだり AI が走ったりはしない = UE5 Editor と同じ挙動
		scene_->UpdateCamera();
		scene_->LateUpdate();
	}

	// --- (2) Scene の3段階更新 (Play/Stop 判定あり) ---
	const bool isPlaying =
		(playModeState_ == nullptr) || playModeState_->IsPlaying();

	if (scene_) {
		// ★ UpdateCamera は Play/Stop に関わらず常に実行
		//    = カメラ操作(DebugCamera含む)は停止中でも可能
		//    = キャラ位置に依存しない基本カメラ処理は維持される
		scene_->UpdateCamera();

		if (isPlaying) {
			// 再生中: ゲームロジックと後処理を実行
			scene_->Update();
			scene_->LateUpdate();
		}
		// 停止中:
		// - Update 呼ばない → キャラ・AI・UI・入力処理すべて停止
		// - LateUpdate 呼ばない → カメラ追従もキャラ位置に追従しない
		//   (UpdateCamera で毎フレーム「定位置のカメラ」を再計算してもらう想定)
	}
}

void SceneManager::Draw()
{
	if (!scene_) return;
	scene_->BackGroundDraw();
	scene_->Draw();
	scene_->ForeGroundDraw();
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);

	// ★ 現在アクティブになるシーン名を記録
	//    (次フレームのUpdateで実際に切り替わったときに参照される)
	currentSceneName_ = sceneName;
}

void SceneManager::ReloadCurrentScene()
{
	// 現在のシーン名が記録されていなければ何もしない
	if (currentSceneName_.empty()) {
		return;
	}

	// 既に次シーンが予約されているなら、まずそれを無視する
	// (Play直後にもう一度Playを押す等の多重呼び出し対策)
	if (nextScene_) {
		delete nextScene_;
		nextScene_ = nullptr;
	}

	// 同じ名前のシーンを再生成
	ChangeScene(currentSceneName_);
}

void SceneManager::RequestChangeScene(const std::string& nextSceneName, const TransitionRequest& req)
{
	assert(transitionService_);

	// すでに次シーンが予約されているなら無視
	if (nextScene_ != nullptr) {
		return;
	}

	// 遷移中なら無視
	if (transitionService_->IsTransitioning()) {
		return;
	}

	// Serviceに委譲
	transitionService_->StartTransition(
		nextSceneName,
		req,
		[this](const std::string& name) { this->ChangeScene(name); },
		[]() {}
	);
}

bool SceneManager::IsTransitioning() const
{
	if (!transitionService_) return false;
	return transitionService_->IsTransitioning();
}