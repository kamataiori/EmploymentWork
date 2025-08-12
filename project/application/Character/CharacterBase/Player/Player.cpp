#include "Player.h"

Player::Player(BaseScene* scene) : scene_(scene) {
    
}

void Player::Initialize(FollowCamera* camera) {
    
    camera_ = camera;

    // 全プレイヤー生成
    // 実体を作る
    warrior_ = std::make_unique<PlayerWarrior>(scene_);
    rogue_ = std::make_unique<PlayerRogue>(scene_);

    // ★先に currentPlayer_ を決める（null のまま使わない）
    currentPlayer_ = warrior_.get();

    // ★コントローラを両者に配線（切替後も安全）
    warrior_->SetAnimationController(&anim_);
    rogue_->SetAnimationController(&anim_);

    // ★現プレイヤーに合わせてマッピングを構築
    anim_.UseWarrior(warrior_->GetAnimationSet());

    // ここから初期化
    warrior_->Initialize();
    rogue_->Initialize();

    // ここでコントローラ配線（どちらにも渡しておくと切替後も安全）
    warrior_->SetAnimationController(&anim_);
    rogue_->SetAnimationController(&anim_);

    // そして初期プレイヤーに合わせてマッピングを構築
    anim_.UseWarrior(warrior_->GetAnimationSet());

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

    switch (type) {
    case PlayerType::Warrior: currentPlayer_ = warrior_.get(); break;
    case PlayerType::Rogue:   currentPlayer_ = rogue_.get();   break;
    }

    if (currentPlayer_) {
        // Transform復元 → 再初期化（2回目以降はTransform初期化しない）
        currentPlayer_->SetTranslate(oldT.translate);
        currentPlayer_->SetRotate(oldT.rotate);
        currentPlayer_->SetScale(oldT.scale);
        currentPlayer_->Initialize();

        // ★ コントローラを差し替え
        currentPlayer_->SetAnimationController(&anim_);

        // ★ マッピングを切り替え
        if (type == PlayerType::Warrior) {
            anim_.UseWarrior(warrior_->GetAnimationSet());
        }
        else {
            // Rogue は RogueAnimationSet を公開するゲッターを用意してください
            // 例: const RogueAnimationSet& GetRogueAnimSet() const { return animation_; }
            anim_.UseRogue(rogue_->GetRogueAnimSet());
        }

        // カメラ追従も切替
        currentPlayer_->SetCamera(camera_);
        camera_->SetTarget(currentPlayer_);
    }

    currentType_ = type;
}


CharacterBase* Player::GetCurrentCharacter() const {
    return currentPlayer_;
}
