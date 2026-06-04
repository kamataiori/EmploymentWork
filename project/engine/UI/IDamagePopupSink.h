// IDamagePopupSink.h
#pragma once
#include "Vector3.h"

//======================================================
// IDamagePopupSink
//------------------------------------------------------
// 敵が被ダメージしたときに「ダメージ数値ポップアップ」を出すための注入口。
// 敵クラス（Enemy / MinionEnemy）は具体的な表示実装を知らずに、
// このインターフェイス越しにダメージ量と発生ワールド座標を通知する。
// 実装は DamagePopupManager（Scene が所有）。
//======================================================
class IDamagePopupSink {
public:
    virtual ~IDamagePopupSink() = default;

    /// <summary>
    /// ダメージ数値の表示を要求する。
    /// </summary>
    /// <param name="worldPos">表示の基準となるワールド座標（敵の中心など）</param>
    /// <param name="amount">与えたダメージ量</param>
    virtual void SpawnDamage(const Vector3& worldPos, int amount) = 0;
};
