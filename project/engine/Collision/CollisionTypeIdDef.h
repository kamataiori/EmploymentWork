#pragma once
#include <cstdint>

enum class CollisionGroup : uint8_t
{
	Default = 0,
	Player = 1,
	Enemy = 2,
	Object = 3,
	World = 4,
};

constexpr uint32_t MakeType(CollisionGroup g, uint16_t local)
{
	return (static_cast<uint32_t>(g) << 16) | static_cast<uint32_t>(local);
}

constexpr CollisionGroup GetGroup(uint32_t type)
{
	return static_cast<CollisionGroup>((type >> 16) & 0xFF);
}

// コリジョン種別ID定義
enum class CollisionTypeIdDef : uint32_t
{
    kDefault = MakeType(CollisionGroup::Default, 0),

    // Player
    kPlayer = MakeType(CollisionGroup::Player, 0),
    PlayerBullet = MakeType(CollisionGroup::Player, 1),
    kPlayerAttack = MakeType(CollisionGroup::Player, 2),
    kPlayerWeapon = MakeType(CollisionGroup::Player, 3),

    // Enemy
    kEnemy = MakeType(CollisionGroup::Enemy, 0),
    EnemyBullet = MakeType(CollisionGroup::Enemy, 1),
    EnemyAreaAttack = MakeType(CollisionGroup::Enemy, 2),

    // World（ステージ地形。メッシュBVHで押し戻す静的コライダー）
    Stage = MakeType(CollisionGroup::World, 0),

    // 物理プロキシ（押し戻し専用のボディ。ヒット判定用ボディとは分離する）
    // Object グループは Stage(World) とだけ当たる（下の ShouldIgnoreCollision 参照）。
    kPlayerBody = MakeType(CollisionGroup::Object, 0),
    kEnemyBody = MakeType(CollisionGroup::Object, 1),
};

// CollisionTypeIdDef からもグループが取れるようにする
constexpr CollisionGroup GetGroup(CollisionTypeIdDef type)
{
    return GetGroup(static_cast<uint32_t>(type));
}
