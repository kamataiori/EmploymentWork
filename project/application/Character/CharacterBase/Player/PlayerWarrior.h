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

	// アニメーションセットを外部から差し替える
	void SetAnimationNames() override;

	// アニメーションセットを取得（ChangePlayer用）
	const AnimationSet& GetAnimationSet() const {
		return animation_;
	}

protected:
    // Warrior モデルの名前を返す
    const char* GetModelName() const override {
        return "Warrior.gltf";
    }

	const AnimationSet& GetAnimation() const override { return animation_; }

private:

	// アニメーションの名前
	AnimationSet animation_;
};
