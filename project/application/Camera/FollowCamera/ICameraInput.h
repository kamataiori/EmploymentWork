#pragma once
#include "InputAction.h"   // LookAxisValue

//======================================================
// ICameraInput
//------------------------------------------------------
// 追従カメラ（FollowCamera）が必要とする視点入力の読み取り口。
//
// なぜインターフェースを挟むか:
//   実装はシーン側の GameSceneInput だが、Camera 層が Scene 層のクラスを
//   直接 include すると依存が逆向きになる。
//   「カメラが欲しい入力はこれだけ」という口をこちら側で決めておく。
//   プレイヤー向けの IPlayerInput と同じ考え方で、読む側ごとに口を分けている
//   （カメラは移動や攻撃を知る必要がなく、プレイヤーは視点感度を知る必要がない）。
//
// 値は「今フレームの変化量」。実装側が毎フレーム先頭で1回更新する。
//======================================================
class ICameraInput
{
public:
	virtual ~ICameraInput() = default;

	/// 周回角（ヨー）の入力。右方向が正
	virtual LookAxisValue GetYawLook() const = 0;

	/// 見下ろし角（ピッチ）の入力。マウスを下げる＝見下ろす向きが正
	virtual LookAxisValue GetPitchLook() const = 0;
};
