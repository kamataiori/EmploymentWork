#pragma once
#include <memory>
#include "PlayerBase.h"
#include "PlayerChange.h"
#include "FollowCamera.h"
#include "PlayerType.h"

class Player {
public:

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Player(BaseScene* scene);

    /// <summary>
    /// 初期化処理。カメラを受け取り、初期プレイヤーを生成
    /// </summary>
    /// <param name=\"camera\">FollowCamera（Scene側で生成）</param>
    void Initialize(FollowCamera* camera);

    /// <summary>
    /// プレイヤーの更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 通常描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// スキニングモデル用の描画処理
    /// </summary>
    void SkinningDraw();


    void ParticlDraw();

    /// <summary>
    /// 当たり判定ヒット時の応答処理
    /// </summary>
    void OnCollision();

    /// <summary>
    /// プレイヤーを別の種類に切り替える
    /// </summary>
    /// <param name=\"type\">新しいプレイヤーの種類</param>
    void ChangePlayer(PlayerType type);

    /// <summary>
    /// 現在のプレイヤー（PlayerBase）を取得
    /// </summary>
    /// <returns>PlayerBase*（主に外部からアクセス用）</returns>
    PlayerBase* Get() const { return currentPlayer_.get(); }

    CharacterBase* GetCurrentCharacter() const;


private:

    // Sceneへの参照
    BaseScene* scene_ = nullptr;
    // 追従カメラ（Scene側で管理）
    FollowCamera* camera_ = nullptr;
    // 現在のプレイヤー
    std::unique_ptr<PlayerBase> currentPlayer_;
    std::unique_ptr<PlayerChange> changer_;

    // 現在のプレイヤーの種類
    PlayerType currentType_ = PlayerType::Warrior;
};