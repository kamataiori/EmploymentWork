#pragma once
#include "Vector3.h"
#include <vector>

//======================================================
// ITarget
//------------------------------------------------------
// プレイヤーの攻撃対象（ボス Enemy・雑魚 MinionEnemy）の共通インターフェース。
//   突進乱舞（RushSlashSkill）などが「敵の具体型を知らずに」
//   位置取得・生存確認・ダメージ適用だけを行えるようにするための抽象。
//======================================================
class ITarget {
public:

	virtual ~ITarget() = default;

	/// <summary>
	/// 生存しているか（死亡・撃破演出中なら false）。攻撃対象の選別に使う。
	/// </summary>
	virtual bool IsAlive() const = 0;

	/// <summary>
	/// 突進先・最近傍計算に使うワールド座標（足元ではなく胴体中心あたり）。
	/// </summary>
	virtual Vector3 GetTargetCenter() const = 0;

	/// <summary>
	/// ダメージを与える（量は呼び出し側が決める）。
	/// </summary>
	virtual void ApplyDamage(int amount) = 0;
};

//======================================================
// IEnemyTargetProvider
//------------------------------------------------------
// 「今この瞬間に生存している攻撃対象の一覧」を供給する。
//   Scene がボス Enemy（自分＋配下の雑魚を束ねる）を実装として注入する想定。
//   突進乱舞はこれを通じて毎回ターゲットを取り直すので、
//   雑魚の出現/撃破にも追従できる。
//======================================================
class IEnemyTargetProvider {
public:

	virtual ~IEnemyTargetProvider() = default;

	/// <summary>
	/// 生存している攻撃対象を out へ追加する（クリアは呼び出し側が行う）。
	/// </summary>
	virtual void CollectAliveTargets(std::vector<ITarget*>& out) = 0;
};
