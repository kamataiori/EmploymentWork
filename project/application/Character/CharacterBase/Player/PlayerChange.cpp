#include "PlayerChange.h"
#include "PlayerWarrior.h"
#include "PlayerRogue.h"

void PlayerChange::ChangeModel(PlayerBase* player, PlayerType type) {
    if (!player) return;

    // モデル名の設定（プレイヤー側に切り替え処理を任せる）
    switch (type) {
    case PlayerType::Warrior:
        player->ChangeModel("Warrior.gltf");
        break;
    case PlayerType::Rogue:
        player->ChangeModel("Rogue.gltf");
        break;
    default:
        break;
    }
    // ここでアニメーション名を再設定
    //player->SetAnimationNames();
}
