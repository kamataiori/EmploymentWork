#pragma once
#include "Vector3.h"
#include <memory>
#include <vector>

class BaseScene;
class Camera;
class CollisionManager;
class IDamagePopupSink;
class ITarget;
class Sentinel;

//======================================================
// SentinelField（前哨の敵の配置・管理）
//------------------------------------------------------
// ステージ中央を囲む正方形の四隅に Sentinel を配置し、
// まとめて更新・描画・当たり判定登録する。
// 全滅（AllDefeated）を State が監視し、ボス登場へ遷移する。
//======================================================
class SentinelField
{
public:
    SentinelField();
    // Sentinel は前方宣言なので、unique_ptr のデストラクタを .cpp 側（完全型が見える場所）に置く
    ~SentinelField();

    // center を中心に、一辺 halfExtent*2 の正方形の四隅へ4体配置する
    void Initialize(BaseScene* scene, const Vector3& center, float halfExtent);

    void SetCamera(Camera* camera);
    void SetDamagePopupSink(IDamagePopupSink* sink);

    // 全員を地中へ沈めて登場を待たせる（イントロ中は見えない）
    void SetHiddenUnderground();
    // 全員の迫り上がりを開始する（SentinelBattleState の入りで呼ぶ）
    void StartSpawn();

    void Update();
    void Draw();
    void ParticleDraw();
    void ForeGroundDraw();

    // 生存中の個体のコライダー（BVH＋被弾スフィア）を登録する
    void RegisterColliders(CollisionManager* cm);

    // 生存中の個体を攻撃対象として集める（プレイヤーのターゲット供給用）
    void CollectAliveTargets(std::vector<ITarget*>& out);

    int  AliveCount() const;
    bool AllDefeated() const;

private:
    std::vector<std::unique_ptr<Sentinel>> sentinels_;
};
