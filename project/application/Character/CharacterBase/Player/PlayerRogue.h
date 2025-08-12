#pragma once
#include "PlayerBase.h"
#include "RogueAnimationSet.h"

class PlayerRogue : public PlayerBase
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

    /// <summary>Rogue用の生アニメセット（Blender名そのまま）を返す</summary>
    const RogueAnimationSet& GetRogueAnimSet() const { return rogueAnim_; }

protected:
    // Rogue モデルの名前を返す
    const char* GetModelName() const override {
        return "Rogue.gltf";
    }

    const AnimationSet& GetAnimation() const override {
        return baseAnim_;
    }



private:

    // Rogue専用：Blender での実アニメ名の集合（Idle/Run など）
    RogueAnimationSet rogueAnim_;

    // 共通キー（AnimationSet）に写したマップ（Run_Weapon が無い場合は Run に寄せる等）
    AnimationSet baseAnim_;
};
