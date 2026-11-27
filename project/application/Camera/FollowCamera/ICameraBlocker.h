#pragma once
#include "Vector3.h"

//======================================================
// ICameraBlocker
//------------------------------------------------------
// 追従カメラを遮る形状（＝ステージの壁や地形）の供給元。
//
// なぜインターフェースを挟むか:
//   遮る形状を持っているのは Scene 側（SceneController が焼いた MeshBVH）だが、
//   Camera 層が Scene 層を直接 include すると依存が逆向きになる。
//   「カメラが知りたいのは『この腕の長さまで伸ばして大丈夫か』だけ」なので、
//   その問いだけを口にしてある。IPlayerInput / ICameraInput と同じ考え方。
//
// 線（レイ）ではなく球で掃く理由:
//   カメラにはニアクリップ面という体積がある。線で判定すると
//   「線は壁に触れていないが画面には壁の内側が映る」状態が残り、
//   柱の角や柵の隙間も線だとすり抜けてしまう。
//======================================================
class ICameraBlocker
{
public:
	virtual ~ICameraBlocker() = default;

	/// <summary>
	/// pivot から desired へ半径 radius の球を掃き、遮られたら true。
	/// </summary>
	/// <param name="pivot">腕の根元（注視点）</param>
	/// <param name="desired">本来置きたいカメラ位置</param>
	/// <param name="radius">カメラの太さ（ニアクリップより大きくする）</param>
	/// <param name="outHitPos">最初に触れた位置（true のときだけ有効）</param>
	virtual bool SweepCamera(const Vector3& pivot, const Vector3& desired,
		float radius, Vector3& outHitPos) const = 0;
};
