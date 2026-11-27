#pragma once

//======================================================
// IPlayerSkill
//------------------------------------------------------
// プレイヤースキルの共通インターフェース。
//   1スキル = 1実装クラス（例: SpinSlashSkill）。
//   PlayerWeapon がスキルを所有し、入力に応じて Start / Update する。
//
// 「スキルの振り付け（choreography）」を実装側に閉じ込め、
// PlayerWeapon は“いつ起動し・何を禁止するか”の調停だけを担う。
//======================================================
class IPlayerSkill {
public:

	virtual ~IPlayerSkill() = default;

	/// <summary>
	/// スキルを発動する（既に発動中なら呼び出し側で弾く想定）。
	/// </summary>
	virtual void Start() = 0;

	/// <summary>
	/// 毎フレーム更新。dt は TimeManager の経過秒。
	/// </summary>
	virtual void Update(float dt) = 0;

	/// <summary>
	/// 発動中か（排他制御・移動アニメ上書き判定などに使う）。
	/// </summary>
	virtual bool IsActive() const = 0;

	/// <summary>
	/// 前景（2D UI）の描画。ロックオン枠などUIを持つスキルだけが上書きする。
	/// デフォルトは何もしない（大半のスキルはUIを持たない）。
	/// </summary>
	virtual void Draw() {}
};
