#include "EnemyState_Dash.h"
#include "Enemy.h"
#include "EnemyState_Idle.h"

void EnemyState_Dash::Enter(Enemy* enemy) {
    // プレイヤー方向ベクトルを取得
    Vector3 toPlayer = enemy->GetPlayerPos() - enemy->GetTransform().translate;

    // Y方向の成分をカットして地面と水平にする
    toPlayer.y = 0.0f;

    // 正規化
    if (Length(toPlayer) < 0.001f) {
        direction = { 0.0f, 0.0f, 1.0f }; // 仮の前方（Z+）
    }
    else {
        direction = Normalize(toPlayer);
    }

    timer = 0.0f;
}

void EnemyState_Dash::Update(Enemy* enemy) {
    // 現在の位置を取得・加算して再代入
    Vector3 pos = enemy->GetTransform().translate;
    pos += direction;
    enemy->SetTranslate(pos);

    // タイマー更新
    timer += 1.0f / 60.0f;
    if (timer >= dashTime) {
        enemy->ChangeToRandomState();
    }
}
