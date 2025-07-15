#include "Player.h"

Player::Player(BaseScene* scene) : scene_(scene) {
    
    // プレイヤー切り替え処理を行うクラスを生成
    changer_ = std::make_unique<PlayerChange>();
}

void Player::Initialize(FollowCamera* camera) {
    
    camera_ = camera;

    // 最初だけプレイヤー生成（Rogue, Warriorなど）
    currentPlayer_ = std::make_unique<PlayerWarrior>(scene_);
    currentPlayer_->Initialize();
    currentPlayer_->SetCamera(camera_);
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
    
    // モデルだけを切り替える（object3d_はそのまま）
    changer_->ChangeModel(currentPlayer_.get(), type);
    currentPlayer_->SetAnimationNames();
    // カメラの再設定（必須なら）
    if (camera_) {
        currentPlayer_->SetCamera(camera_);
    }

    // 現在の種類を記録
    currentType_ = type;
}

CharacterBase* Player::GetCurrentCharacter() const
{
    return currentPlayer_.get();  // currentPlayer_ は CharacterBase を継承している
}
