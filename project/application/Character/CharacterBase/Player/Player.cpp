#include "Player.h"

Player::Player(BaseScene* scene) : scene_(scene) {
    
}

void Player::Initialize(FollowCamera* camera) {
    
    camera_ = camera;

    // 全プレイヤー生成
    // 実体を作る
    warrior_ = std::make_unique<PlayerWarrior>(scene_);
    rogue_ = std::make_unique<PlayerRogue>(scene_);

    // 先に現在プレイヤーを決める
    currentPlayer_ = warrior_.get();

    // 先に両者Initialize（モデル+アニメ読み込み可能状態）
    warrior_->Initialize();
    rogue_->Initialize();

    // どちらにもコントローラ配線（切替後も安全）
    warrior_->SetAnimationController(&anim_);
    rogue_->SetAnimationController(&anim_);

    // 初期はWarriorマッピングを使用
    anim_.Use(PlayerType::Warrior);

    // カメラ
    warrior_->SetCamera(camera_);
    rogue_->SetCamera(camera_);
    camera_->SetTarget(currentPlayer_);

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

    Transform oldT;
    if (currentPlayer_) oldT = currentPlayer_->GetTransform();

    // 切り替え
    currentType_ = type;
    currentPlayer_ = (type == PlayerType::Warrior) ? static_cast<PlayerBase*>(warrior_.get())
        : static_cast<PlayerBase*>(rogue_.get());

    // Transform復元 → 再初期化
    currentPlayer_->SetTranslate(oldT.translate);
    currentPlayer_->SetRotate(oldT.rotate);
    currentPlayer_->SetScale(oldT.scale);
    currentPlayer_->Initialize();

    // コントローラは既に配線済み。マッピングだけ差し替え
    anim_.Use(type);

    // カメラ追従
    currentPlayer_->SetCamera(camera_);
    camera_->SetTarget(currentPlayer_);
}


CharacterBase* Player::GetCurrentCharacter() const {
    return currentPlayer_;
}
