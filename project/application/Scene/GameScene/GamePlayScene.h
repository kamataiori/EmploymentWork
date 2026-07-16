#pragma once
#include <Vector2.h>
#include <Vector3.h>
#include <OffscreenRendering.h>   // PostEffectType
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
#include "BattleTargets.h"
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

	// プレイヤーの攻撃対象の供給元。上のエンティティ群を束ね、
	// その局面で本当に殴れる敵だけを返す（参照のみ保持するので生成はこの後）。
	std::unique_ptr<BattleTargets> battleTargets_;

	std::unique_ptr<CollisionManager> collisionManager_;
	std::unique_ptr<SceneController>  stage_;
	std::unique_ptr<UIManager>        uiManager_;

	// 敵への与ダメージ数値ポップアップ（敵へ注入する）
	std::unique_ptr<DamagePopupManager> damagePopup_;

	// PauseScreen の所有は uiManager_。こちらは状態監視用の非所有参照。
	PauseScreen* pauseScreenRef_ = nullptr;
	bool wasPausedLastFrame_ = false;

	// 戦いの場を手前・奥で分ける。
	//   手前：円状のコロシアム。ここで第1波の群れと戦う。
	//   奥　：四角のエリア。細い道を通って到達すると前哨戦（Sentinel）が始まる。
	// ステージ座標系ではプレイヤー初期位置が手前(-Z)、ボスが奥(+Z)。
	static constexpr float kFrontArenaCenterZ_ = -10.0f;  // 手前の円の中心Z（群れのスポーン中心）
	static constexpr float kBackArenaCenterZ_  = 280.0f;  // 奥の四角の中心Z（前哨戦の場）
	static constexpr float kSwarmRingRadius_   =  12.0f;  // 群れが湧く円の半径

	// 前哨の敵の配置：奥エリア中心から四隅までのXZ距離
	static constexpr float kSentinelFieldHalfExtent_ = 32.0f;

	// 中央コア（ボスのコア）のスケール。四隅より大きく見せて中心を強調する
	static constexpr float kCenterCoreScale_ = 4.0f;

	// ===== 被弾時の赤いヴィネット =====
	// World 層（背景＋3Dオブジェクト＋前景スプライト）に掛ける。
	// UI（UIDraw）は合成後のバックバッファへ直接描くので元から影響を受けない。
	//
	// 1レイヤーにつき同時に掛けられるパスは1つ（OffscreenRendering::SetPostEffectType が
	// チェーンを差し替える作り）なので、被弾中だけ Vignette へ差し替え、明けたら戻す。
	static constexpr PostEffectType kWorldPostEffect_ = PostEffectType::Normal; // 平常時の World 層
	static constexpr float kDamageVignetteDuration_ = 0.35f;  // 赤みが出てから引くまで（秒）
	// Vignette.PS.hlsl: vignette = saturate(pow(correct * scale, power)) を色との lerp 係数に使う。
	// power=0 なら係数が全面1＝素通り（無効）、上げるほど周囲が色に寄る。ここを 0→peak→0 で振る。
	// scale を上げるほど「素通りになる中心の範囲」が広がり、赤みが画面の縁へ寄る
	static constexpr float kDamageVignetteScale_    = 22.0f;
	// power を下げるほど係数が全体的に1へ寄る＝赤みが薄くなる
	static constexpr float kDamageVignettePeakPow_  = 0.5f;
	// 周囲に乗る色。純赤(1,0,0)はきつく出るので、少し沈めた赤にする
	static constexpr Vector3 kDamageVignetteColor_{ 0.85f, 0.06f, 0.1f };

	int   lastPlayerHp_ = -1;             // 前フレームのHP（減ったら被弾とみなす）
	float damageVignetteTimer_ = 0.0f;    // 残り時間（0=出ていない）
	bool  damageVignetteActive_ = false;  // World 層を Vignette へ差し替え中か

	// 被弾の検知とヴィネットの増減。毎フレーム呼ぶ
	void UpdateDamageVignette();

	// 天球（skydome.obj）のスケール。モデルはスケール1で半径100。
	// 戦場は原点から最も遠い点で約330（奥アリーナ Z=280 ＋ 四隅 45）なので、
	// それを余裕をもって包める半径500まで広げる。
	static constexpr float kSkydomeScale_ = 5.0f;
	// カメラの遠クリップ。既定は100しかなく、それだと天球（半径500）が丸ごと
	// 切り捨てられて映らない。天球の反対側まで見通せる距離を確保する。
	static constexpr float kCameraFarClip_ = 2000.0f;

	// 中央コアの弱点（被弾スフィア・狙う点・コアの光）の位置。
	// EnemyCore.obj はスケール1で 幅3.35×高さ3.57×奥行3.76、手前(-Z)の面は z=-1.71。
	// スケール4だと面は z=-6.83 なので、弱点を原点に置くとメッシュの奥深くに埋まり、
	// プレイヤーから見えない上に BVH に阻まれて剣も届かない。面の外へ出しておく。
	// 高さは剣（プレイヤーの手のジョイントに追従）が届く低さにする。
	static constexpr float kCoreWeakPointForwardZ_ = -7.5f; // 手前(-Z)へどれだけ出すか
	static constexpr float kCoreWeakPointHeightY_  =  3.0f; // 弱点の高さ

	// Bloom調整用（ImGui）
	float bloomThreshold_ = 0.8f;
	float bloomIntensity_ = 1.2f;
	int   bloomIterations_ = 5;
};
