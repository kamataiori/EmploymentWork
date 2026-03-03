#include "EnemySplitBullet.h"
#include "engine/TimeManager.h"
#include <cmath>

static Vector3 NormalizeSafe(const Vector3& v)
{
    float len = Length(v);
    if (len < 1e-6f) return { 0,0,1 };
    return v / len;
}

void EnemySplitBullet::InitializeBurst(
    const Vector3& startPos,
    const Vector3& lockPlayerPos,
    float riseHeight,
    float riseSpeed,
    float splitRadius,
    float shotSpeed
)
{
    startPos_ = startPos;
    lockPlayerPos_ = lockPlayerPos;

    riseHeight_ = riseHeight;
    riseSpeed_ = riseSpeed;
    splitRadius_ = splitRadius;
    shotSpeed_ = shotSpeed;

    transform.translate = startPos_;
    transform.rotate = { 0,0,0 };
    transform.scale = { 1,1,1 };

    // collider（Sphere1個）
    multiCollider_->Clear();

    Sphere sp{};
    sp.center = transform.translate;
    sp.radius = radius_;
    multiCollider_->AddSphere(sp);
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyBullet));
    multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });

    phase_ = Phase::Rise;
    isDead_ = false;

    //==========================
    // Particle 初期化（deathSystem_と同じ）
    //==========================
    particleSystem_ = std::make_unique<ParticleManager>();
    particleSystem_->Initialize(VertexDataType::Plane);

    particleSystem_->LoadAllPresets();
    particleSystem_->LoadAllSystems();

    // bulletのカメラをParticleにも渡す（Enemy側でSetCameraされるのでここで1回だけでもOK）
    if (camera_) {
        particleSystem_->SetCamera(camera_);
    }

    particleTransform_ = transform;

    // スポーン直後に1回Emit（見た目の初速）
    particleTransform_.translate = transform.translate;
    //particleSystem_->EmitSystemByName(particleSystemName_, particleTransform_);

    emitTimer_ = 0.0f;
}

void EnemySplitBullet::Update()
{
    float dt = TimeManager::GetInstance()->GetDeltaTime();

    if (phase_ == Phase::Rise)
    {
        transform.translate.y += riseSpeed_ * dt;

        // 一定高さへ到達したら分裂配置へ移動開始
        if (transform.translate.y >= startPos_.y + riseHeight_)
        {
            // 分裂の4点（十字）: (+x, -x, +z, -z)
            Vector3 base = transform.translate; // 到達高度の位置が基準（真上）
            switch (index_)
            {
            case 0: splitTargetPos_ = base + Vector3{ +splitRadius_, 0.0f, 0.0f }; break;
            case 1: splitTargetPos_ = base + Vector3{ -splitRadius_, 0.0f, 0.0f }; break;
            case 2: splitTargetPos_ = base + Vector3{ 0.0f, 0.0f, +splitRadius_ }; break;
            case 3: splitTargetPos_ = base + Vector3{ 0.0f, 0.0f, -splitRadius_ }; break;
            default: splitTargetPos_ = base; break;
            }

            phase_ = Phase::SplitMove;

            //// 分裂の瞬間に少し強めに出したいならここでEmit
            //particleTransform_.translate = transform.translate;
            //particleSystem_->EmitSystemByName(particleSystemName_, particleTransform_);
        }
    }
    else if (phase_ == Phase::SplitMove)
    {
        // 分裂後の所定位置へスッと移動
        Vector3 to = splitTargetPos_ - transform.translate;
        float d = Length(to);

        if (d < 0.05f)
        {
            transform.translate = splitTargetPos_;

            // ここで「プレイヤーへ向けた方向」をロックして直線発射
            Vector3 toP = lockPlayerPos_ - transform.translate;
            shotDir_ = NormalizeSafe(toP);

            phase_ = Phase::Shot;
        }
        else
        {
            Vector3 dir = NormalizeSafe(to);
            float step = splitMoveSpeed_ * dt;
            if (step > d) step = d;
            transform.translate += dir * step;
        }
    }
    else // Shot
    {
        transform.translate += shotDir_ * (shotSpeed_ * dt);

        // 寿命（暫定）：遠くに行ったら消す
        // ※本当はlifeTimerか画面外判定にするのがいい
        if (Length(transform.translate - startPos_) > 200.0f)
        {
            isDead_ = true;
        }
    }

    // collider 更新
    Sphere& sp = multiCollider_->MutableSphere(0);
    sp.center = transform.translate;
    sp.radius = radius_;

    //==========================
    // Particle 更新（追従トレイル）
    //==========================
    if (particleSystem_)
    {
        emitTimer_ += dt;
        if (emitTimer_ >= emitInterval_)
        {
            emitTimer_ = 0.0f;

            particleTransform_.translate = transform.translate;
            particleSystem_->EmitSystemByName(particleSystemName_, particleTransform_);
        }

        particleSystem_->Update();
    }
}

void EnemySplitBullet::Draw()
{
    // デバッグ球だけ
    multiCollider_->Draw();
}

void EnemySplitBullet::ParticleDraw()
{
    if (particleSystem_) {
        OutputDebugStringA("EnemySplitBullet::ParticleDraw\n");
        particleSystem_->Draw();
    }
}

void EnemySplitBullet::OnCollision(const CollisionInfo& info)
{
    // プレイヤーに当たったら消す（あなたのTypeIdに合わせて調整）
    auto other = static_cast<CollisionTypeIdDef>(info.otherType);

    if (other == CollisionTypeIdDef::kPlayer ||
        other == CollisionTypeIdDef::kPlayerWeapon ||
        other == CollisionTypeIdDef::kPlayerAttack ||
        other == CollisionTypeIdDef::PlayerBullet)
    {
        isDead_ = true;
    }
}

void EnemySplitBullet::SetCamera(Camera* camera)
{
    ObjectBase::SetCamera(camera);
    if (particleSystem_) {
        particleSystem_->SetCamera(camera);
    }
}
