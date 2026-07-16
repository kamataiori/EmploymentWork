#pragma once
#include <Vector2.h>
#include <vector>
#include <memory>
#include <Sprite.h>
#include <Object3d.h>
#include "BaseScene.h"
#include "Audio.h"
#include "Light.h"
#include "ParticleManager.h"
#include "Player.h"
#include "DrawLine.h"
#include "CollisionManager.h"
#include <Enemy/Enemy.h>
#include <Enemy/Sentinel/Sentinel.h>
#include <Enemy/Sentinel/SentinelField.h>
#include <Enemy/Swarm/SwarmController.h>
#include <FollowCamera.h>
#include "Camera/CameraEffectController.h"
#include "SkyBox.h"
#include "engine/UI/UIManager.h"
#include "engine/UI/DamagePopupManager.h"
#include "PauseScreen.h"
#include "Flow/GameFlowContext.h"
#include "Flow/GameFlowStateMachine.h"

//======================================================
// GamePlayScene
//------------------------------------------------------
// エンティティ・カメラ・UI の「所有者」。
// バトルがどう進むか（登場演出 → 戦闘 → 決着）は GameFlowStateMachine が持ち、
// このクラスの Update/描画は、そのときの局面へ丸ごと委譲するだけの薄い層にする。
//
// 局面を1つ増やしたいときは Flow/States/ に State を足して繋ぐだけで、
// このクラスは触らずに済む。
//======================================================
class GamePlayScene : public BaseScene
{
public:
	void Initialize() override;
	void Finalize() override;

	// 新しいライフサイクル (BaseScene)
	void UpdateCamera() override;   // カメラを最初に更新
	void Update() override;         // ゲームロジック
	void LateUpdate() override;     // 後処理(カメラ追従・カメラエフェクト)

	void BackGroundDraw() override;
	void Draw() override;
	void ForeGroundDraw() override;

	// Canvas合成方式を使用（UIを非エフェクト化・パーティクルを専用レイヤーへ分離）
	bool UsesCanvasCompositing() const override { return true; }
	void ParticleDraw() override;   // パーティクル専用Canvasへ
	void UIDraw() override;          // 合成後のバックバッファへ（エフェクト対象外）

	void Debug() override;

	// マウスカーソル設定: ゲームプレイ中は非表示+ウィンドウ内に閉じ込める
	bool ShouldShowCursor() const override { return false; }
	bool ShouldConfineCursor() const override { return true; }

	void SetCamera1(std::unique_ptr<Camera> newCamera) { camera1 = std::move(newCamera); }
	Camera* GetCamera1() const { return camera1.get(); }

private:
	// ================================================
	// バトルの進行（State は Flow/States/ にある）
	// ================================================
	GameFlowStateMachine flow_;
	GameFlowContext      context_{};

	// context_ に各エンティティの非所有ポインタを差し込む（Initialize の最後で呼ぶ）
	void BuildFlowContext();

	// ポーズ状態の変化を見てカーソルを切替える
	void SyncCursorWithPauseState();

	// ================================================

	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();
	std::unique_ptr<FollowCamera> followCamera;
	std::unique_ptr<CameraEffectController> cameraEffect_;

	std::unique_ptr<SkyBox>   skybox = std::make_unique<SkyBox>();
	std::unique_ptr<Object3d> ground;
	std::unique_ptr<Object3d> sky;

	std::unique_ptr<Player>   player_;
	std::unique_ptr<Enemy>    enemy_;

	// 第1波の群れ（波状に登場する近接スウォーム）。
	std::unique_ptr<SwarmController> swarmController_;

	// 前哨の敵4体（四隅）。全滅すると中央コアが破壊可能になる。
	std::unique_ptr<SentinelField> sentinelField_;

	// 中央のボスのコア（Sentinel を流用）。破壊するとボスが登場する。
	std::unique_ptr<Sentinel> centerCore_;

	std::unique_ptr<CollisionManager> collisionManager_;
	std::unique_ptr<SceneController>  stage_;
	std::unique_ptr<UIManager>        uiManager_;

	// 敵への与ダメージ数値ポップアップ（敵へ注入する）
	std::unique_ptr<DamagePopupManager> damagePopup_;

	// PauseScreen の所有は uiManager_。こちらは状態監視用の非所有参照。
	PauseScreen* pauseScreenRef_ = nullptr;
	bool wasPausedLastFrame_ = false;

	// 前哨の敵の配置：中心（原点）から四隅までのXZ距離
	static constexpr float kSentinelFieldHalfExtent_ = 20.0f;

	// 中央コア（ボスのコア）のスケール。四隅より大きく見せて中心を強調する
	static constexpr float kCenterCoreScale_ = 4.0f;

	// Bloom調整用（ImGui）
	float bloomThreshold_ = 0.8f;
	float bloomIntensity_ = 1.2f;
	int   bloomIterations_ = 5;
};
