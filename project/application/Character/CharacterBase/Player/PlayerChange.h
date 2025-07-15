#pragma once
#include <memory>
#include "PlayerBase.h"
#include "PlayerWarrior.h"
#include "PlayerRogue.h"
#include "PlayerType.h"
#include "BaseScene.h"

class PlayerChange {
public:

    PlayerChange() = default;

    // プレイヤーのモデルだけを切り替える（再生成しない）
    void ChangeModel(PlayerBase* player, PlayerType type);

private:

    // プレイヤー生成に使うシーン参照
    BaseScene* scene_ = nullptr;
};