#include "SwarmEnemy.h"
#include "engine/TimeManager.h"
#include "engine/3d/Camera/Camera.h"
#include "engine/UI/IDamagePopupSink.h"
#include <cmath>

namespace {
    Vector3 NormalizeSafe(const Vector3& v)
    {
        const float len = Length(v);
        if (len < 1e-6f) return { 0.0f, 0.0f, 1.0f };
        return v / len;
    }

    float WrapDeltaRad(float a) {
        while (a >  3.1415926535f) a -= 6.283185307f;
        while (a < -3.1415926535f) a += 6.283185307f;
        return a;
    }

    float LerpAngleRad(float from, float to, float t) {
        return from + WrapDeltaRad(to - from) * t;
    }
}

SwarmEnemy::SwarmEnemy(BaseScene* scene)
    : ObjectBase(scene)
{
}

void SwarmEnemy::InitializeSwarm()
{
    ModelManager::GetInstance()->LoadModel(kModelName);

    object3d_->Initialize();
    object3d_->SetModel(kModelName);

    transform.scale = { kModelScale, kModelScale, kModelScale };
    object3d_->SetScale(transform.scale);

    particles_ = std::make_unique<ParticleManager>();
    particles_->Initialize(VertexDataType::Plane);
    particles_->LoadAllPresets();
    if (camera_) {
        particles_->SetCamera(camera_);
    }

    // 本体（Enemyグループ・Trigger）。触れると Player 側の既存処理で被弾する。
    multiCollider_->Clear();
    Sphere sp{};
    sp.radius = kRadius;
    multiCollider_->AddSphere(sp);
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
    multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });
}

void SwarmEnemy::Respawn(const Vector3& spawnPos)
{
    transform.translate = spawnPos;
    transform.translate.y += kGroundOffsetY;
    transform.rotate = { 0.0f, 0.0f, 0.0f };
    groundY_ = transform.translate.y;

    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);

    phase_ = Phase::Chase;
    lungeTimer_ = 0.0f;
    recoverTimer_ = 0.0f;
    isDead_ = false;
    hp_ = kMaxHP_;
    active_ = true;

    // 死亡時に 0 にした当たり半径を戻し、新しい位置へ移す
    if (!multiCollider_->GetShapes().empty()) {
        multiCollider_->MutableSphere(0).center = transform.translate;
        multiCollider_->MutableSphere(0).radius = kRadius;
    }

    // ここで行列を確定させておく。Object3d::Draw は定数バッファを渡すだけで、
    // 中身を書くのは Update なので、これが無いと出現した最初の1フレームだけ
    // 未更新の行列（＝原点）で描かれてしまう。
    object3d_->Update();
}

void SwarmEnemy::Update()
{
    if (isDead_) {
        if (particles_) particles_->Update();
        return;
    }

    const float dt = TimeManager::GetInstance()->GetDeltaTime();

    // プレイヤーへの水平ベクトル
    Vector3 toPlayer{ 0.0f, 0.0f, 0.0f };
    float distToPlayer = 0.0f;
    if (playerTarget_) {
        toPlayer = playerTarget_->translate - transform.translate;
        toPlayer.y = 0.0f;
        distToPlayer = Length(toPlayer);
    }
    const Vector3 dir = NormalizeSafe(toPlayer);

    // 進行方向（追尾・突進中）へ向く
    const bool facePlayer = (phase_ != Phase::Recover);
    if (facePlayer && distToPlayer > 1e-4f) {
        const float desiredYaw = std::atan2(dir.x, dir.z);
        transform.rotate.y = LerpAngleRad(transform.rotate.y, desiredYaw, kTurnLerp);
    }

    if (phase_ == Phase::Chase) {
        // プレイヤーへ寄る。射程に入ったらランジ開始。
        transform.translate.x += dir.x * kChaseSpeed * dt;
        transform.translate.z += dir.z * kChaseSpeed * dt;

        if (playerTarget_ && distToPlayer <= kLungeRange) {
            lungeDir_ = dir;             // 突進方向を確定（以後ホーミングしない）
            lungeTimer_ = 0.0f;
            phase_ = Phase::Lunge;
        }
    }
    else if (phase_ == Phase::Lunge) {
        // 確定方向へ短く突進
        transform.translate.x += lungeDir_.x * kLungeSpeed * dt;
        transform.translate.z += lungeDir_.z * kLungeSpeed * dt;

        lungeTimer_ += dt;
        if (lungeTimer_ >= kLungeDuration) {
            recoverTimer_ = 0.0f;
            phase_ = Phase::Recover;
        }
    }
    else { // Recover
        // プレイヤーと反対へ少し後退して仕切り直す（次のランジまで距離を作る）
        transform.translate.x -= dir.x * kRecoverBackSpeed * dt;
        transform.translate.z -= dir.z * kRecoverBackSpeed * dt;

        recoverTimer_ += dt;
        if (recoverTimer_ >= kRecoverDuration) {
            phase_ = Phase::Chase;
        }
    }

    // 地上に維持
    transform.translate.y = groundY_;

    // コライダー追従
    if (!multiCollider_->GetShapes().empty()) {
        multiCollider_->MutableSphere(0).center = transform.translate;
    }

    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->Update();

    if (particles_) particles_->Update();
}

void SwarmEnemy::Draw()
{
    if (!active_ || isDead_) return;
    object3d_->Draw();
}

void SwarmEnemy::ParticleDraw()
{
    if (particles_) particles_->Draw();
}

void SwarmEnemy::OnCollision(const CollisionInfo& info)
{
    if (isDead_) return;

    const auto other = static_cast<CollisionTypeIdDef>(info.otherType);

    // プレイヤーの攻撃で被弾する
    if (other == CollisionTypeIdDef::kPlayerWeapon ||
        other == CollisionTypeIdDef::kPlayerAttack ||
        other == CollisionTypeIdDef::PlayerBullet)
    {
        ApplyDamage(kDamagePerHit);
        return;
    }

    // プレイヤー本体に当てた（＝体当たり成功）→ 一旦離れて仕切り直す。
    // プレイヤーへのダメージは Player 側の OnCollision が担う。
    if (other == CollisionTypeIdDef::kPlayer && phase_ != Phase::Recover) {
        recoverTimer_ = 0.0f;
        phase_ = Phase::Recover;
    }
}

void SwarmEnemy::ApplyDamage(int amount)
{
    if (isDead_) return;

    if (damageSink_ && amount > 0) {
        damageSink_->SpawnDamage(GetTargetCenter(), amount);
    }

    if (particles_) {
        Transform spark{};
        spark.scale = { 1.0f, 1.0f, 1.0f };
        spark.rotate = { 0.0f, 0.0f, 0.0f };
        spark.translate = transform.translate;
        spark.translate.y += kEffectOffsetY_;
        particles_->EmitPreset(kHitSparkPreset_, spark);
    }

    hp_ -= amount;
    if (hp_ <= 0) {
        isDead_ = true;
        if (!multiCollider_->GetShapes().empty()) {
            multiCollider_->MutableSphere(0).radius = 0.0f;
        }
        if (particles_) {
            Transform burst{};
            burst.scale = { 1.0f, 1.0f, 1.0f };
            burst.rotate = { 0.0f, 0.0f, 0.0f };
            burst.translate = transform.translate;
            burst.translate.y += kEffectOffsetY_;
            particles_->EmitPreset(kExplosionPreset_, burst);
        }
        TimeManager::GetInstance()->RequestHitStop(HitStopPreset::Heavy());
    }
}

void SwarmEnemy::SetCamera(Camera* camera)
{
    ObjectBase::SetCamera(camera);
    if (particles_) {
        particles_->SetCamera(camera);
    }
}
