#include "Player.h"

Player::Player(BaseScene* scene) : scene_(scene) {
    
}

void Player::Initialize(FollowCamera* camera) {
    
    camera_ = camera;

    // 全プレイヤー生成
    warrior_ = std::make_unique<PlayerWarrior>(scene_);
    rogue_ = std::make_unique<PlayerRogue>(scene_);

    warrior_->Initialize();
    warrior_->SetCamera(camera_);
    rogue_->Initialize();
    rogue_->SetCamera(camera_);

    currentPlayer_ = warrior_.get();  // 最初は Warrior
}

void Player::Update() {

    // プレイヤーの切り替え（1キー = Warrior、2キー = Rogue）
    if (Input::GetInstance()->TriggerKey(DIK_1)) {
        ChangePlayer(PlayerType::Warrior);
    }
    if (Input::GetInstance()->TriggerKey(DIK_2)) {
        ChangePlayer(PlayerType::Rogue);
    }

    // 現在のプレイヤーを更新
    if (currentPlayer_) {
        currentPlayer_->Update();
    }
}

void Player::Draw() {
    
    if (currentPlayer_) {
        currentPlayer_->Draw();
    }
}

void Player::SkinningDraw() {
    
    if (currentPlayer_) {
        currentPlayer_->SkinningDraw();
    }
}

void Player::ParticlDraw()
{
    currentPlayer_->ParticleDraw();
}

void Player::OnCollision() {

    if (currentPlayer_) {
        currentPlayer_->OnCollision();
    }
}

void Player::ChangePlayer(PlayerType type) {
    if (currentType_ == type) return;

    Transform oldTransform;
    if (currentPlayer_) {
        oldTransform = currentPlayer_->GetTransform();
    }

    switch (type) {
    case PlayerType::Warrior:
        currentPlayer_ = warrior_.get();
        break;
    case PlayerType::Rogue:
        currentPlayer_ = rogue_.get();
        break;
    }

    if (currentPlayer_) {
        currentPlayer_->SetTranslate(oldTransform.translate);
        currentPlayer_->SetRotate(oldTransform.rotate);
        currentPlayer_->SetScale(oldTransform.scale);

        currentPlayer_->Initialize();  // transform は上で設定済みなので初期化内で再初期化されない

        currentPlayer_->SetCamera(camera_);
        currentPlayer_->SetAnimationNames();
    }

    currentType_ = type;
}

CharacterBase* Player::GetCurrentCharacter() const {
    return currentPlayer_;
}
