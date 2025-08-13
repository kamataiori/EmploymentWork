#pragma once
#include "PlayerBase.h"
#include <AnimationSet.h>

class PlayerWarrior : public PlayerBase
{
public:

    // 基底クラスのコンストラクタを継承
    using PlayerBase::PlayerBase;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

protected:
    // Warrior モデルの名前を返す
    const char* GetModelName() const override {
        return "Warrior.gltf";
    }


};
