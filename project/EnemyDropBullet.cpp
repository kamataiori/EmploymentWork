#include "EnemyDropBullet.h"
#include "engine/TimeManager.h"

void EnemyDropBullet::Initialize()
{
}

void EnemyDropBullet::Initialize(const Vector3& shootPos, const Vector3& targetPos)
{
    startPos_ = shootPos;
    targetPos_ = targetPos;

    transform.translate = startPos_;
    transform.rotate = { 0,0,0 };
    transform.scale = { 1,1,1 };

    // コライダー：Sphere 1つ
    multiCollider_->Clear();

    Sphere sp{};
    sp.center = transform.translate;
    sp.radius = radius_;

    multiCollider_->AddSphere(sp);
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::EnemyBullet));

    // 相手情報付きで弾側のOnCollisionへ橋渡し
    multiCollider_->SetHitCallbackEx([this](const CollisionInfo& info) { this->OnCollision(info); });

    phase_ = Phase::Rise;
    timer_ = 0.0f;
    isDead_ = false;
}

void EnemyDropBullet::Update()
{
    float dt = TimeManager::GetInstance()->GetDeltaTime();

    timer_ += dt;
    if (timer_ >= life_) {
        isDead_ = true;
        return;
    }

    if (phase_ == Phase::Rise) {
        transform.translate.y += riseSpeed_ * dt;

        // apex 到達で落下へ
        if (transform.translate.y >= startPos_.y + apexHeight_) {
            phase_ = Phase::Fall;
        }
    }
    else { // Fall
        // 落下（Y）
        transform.translate.y -= fallSpeed_ * dt;

        // 落下中に XZ を target に寄せる（「真上から狙って落とす」感）
        Vector3 pos = transform.translate;
        Vector3 desired = { targetPos_.x, pos.y, targetPos_.z };

        float t = homingLerp_ * dt;
        if (t > 1.0f) t = 1.0f;
        pos.x = pos.x + (desired.x - pos.x) * t;
        pos.z = pos.z + (desired.z - pos.z) * t;

        transform.translate = pos;

        // 地面に落ちたら消す（暫定：y<=0）
        if (transform.translate.y <= 0.0f) {
            isDead_ = true;
            return;
        }
    }

    // Sphere中心更新
    Sphere& sp = multiCollider_->MutableSphere(0);
    sp.center = transform.translate;
    sp.radius = radius_;
}

void EnemyDropBullet::Draw()
{
    // モデル描画はしない。デバッグ球のみ。
    multiCollider_->Draw();
}

void EnemyDropBullet::OnCollision()
{
    // 何かに当たったら消える（暫定）
    isDead_ = true;
}

void EnemyDropBullet::OnCollision(const CollisionInfo& info)
{
    auto other = static_cast<CollisionTypeIdDef>(info.otherType);

    // プレイヤー関連に当たったら消す（必要に応じて追加）
    if (other == CollisionTypeIdDef::kPlayer ||
        other == CollisionTypeIdDef::kPlayerWeapon ||
        other == CollisionTypeIdDef::kPlayerAttack ||
        other == CollisionTypeIdDef::PlayerBullet)
    {
        isDead_ = true;
        return;
    }

    // それ以外でも消したいならここで
    // isDead_ = true;
}