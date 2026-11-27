#pragma once
#include "Vector3.h"

class Player;
class Enemy;
class Sentinel;
class SentinelField;
class SwarmController;
class SceneController;
class SkyBox;
class Object3d;
class FollowCamera;
class CameraEffectController;
class CollisionManager;
class UIManager;
class DamagePopupManager;
class MissionBanner;
class GameFlowStateMachine;

//======================================================
// GameFlowContext
//------------------------------------------------------
// State が触る「場」。所有はすべて GamePlayScene 側にあり、
// ここに入るのは非所有ポインタだけ。
//
// State に GamePlayScene そのものを渡すと、State からシーンの全メンバーが
// 見えてしまい結局密結合になる。State に必要なものだけをここへ集める。
//======================================================
struct GameFlowContext
{
    // 登場人物
    Player* player = nullptr;
    Enemy*  boss   = nullptr;   // Enemy（ボス）。前哨戦の後に登場する
    SwarmController* swarm = nullptr;    // 第1波の群れ（波状に登場）
    SentinelField* sentinels = nullptr;  // 前哨の敵4体（全滅でコアが破壊可能に）
    Sentinel* centerCore = nullptr;      // 中央のボスのコア（Sentinel全滅で破壊解禁→破壊でボス登場）

    // 背景・ステージ
    SceneController* stage  = nullptr;
    SkyBox*          skybox = nullptr;
    Object3d*        ground = nullptr;
    Object3d*        sky    = nullptr;

    // カメラ
    FollowCamera*           followCamera = nullptr;
    CameraEffectController* cameraEffect = nullptr;

    // 仕組み
    CollisionManager*     collisionManager = nullptr;
    UIManager*            uiManager        = nullptr;
    DamagePopupManager*   damagePopup      = nullptr;
    // 各バトルの開始前に目的の一文を流すバナー（実体は uiManager が所有）
    MissionBanner*        missionBanner    = nullptr;

    // 奥の四角エリア（前哨戦の場）の中心。SwarmWave後、プレイヤーがここへ到達すると
    // 前哨戦（Sentinel）が始まる。Sentinel/中央コア/ボスの配置基準でもある。
    Vector3 backArenaCenter{ 0.0f, 0.0f, 30.0f };

    // 自分が属するステートマシン（State から遷移を要求するのに使う）
    GameFlowStateMachine* flow = nullptr;

    // true の間、FollowCamera の通常追従を止めて CameraEffectController に任せる。
    // 決着演出（オービット）で使う。GamePlayScene::UpdateCamera() が読む。
    bool followCameraLocked = false;
};
