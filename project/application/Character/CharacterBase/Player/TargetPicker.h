#pragma once
#include "ITarget.h"
#include "Vector3.h"
#include <vector>

//======================================================
// TargetPicker
//------------------------------------------------------
// 「基準方向から見て、今いちばん狙っていそうな敵」を1体選ぶ部品。
//
// 最近傍で選ばない理由:
//   目の前の敵を狙いたいのに、背後にいる少し近い敵を選んでしまうため。
//   3Dアクションの定石どおり「角度を距離より優先する」重み付きスコアで選ぶ。
//
// 選び方:
//   1. 射程(maxDistance)と、基準方向からの角度(maxAngleDeg)で候補を絞る
//   2. 残りを score = 角度[度]*angleWeight + 距離[m]*distanceWeight で評価し、最小を選ぶ
//      （angleWeight を効かせるほど「向けている方向」が優先される）
//
// 向きはヨー角しか変えないので、距離も角度も高さを無視した水平面で測る。
//
// 「誰の入力か」は知らない（基準方向は呼び出し側が決める）ので、
// 通常攻撃のソフトロック以外にも使える。
//======================================================
class TargetPicker
{
public:
	//--------------------------------------------------
	// 調整値。実機で詰められるよう ImGui から編集できる。
	//--------------------------------------------------
	struct Params {
		float maxDistance = 8.0f;      // 射程[m]。これより遠い敵は拾わない
		float maxAngleDeg = 70.0f;     // 基準方向からの許容角（片側）[度]
		float angleWeight = 1.0f;      // スコアの重み（角度1度あたり）
		float distanceWeight = 2.0f;   // スコアの重み（距離1mあたり）
	};

	/// <summary>
	/// 条件に合う敵を1体返す（居なければ nullptr）。
	/// </summary>
	/// <param name="provider">生存している敵の供給元（その局面で殴れる敵だけを返す）</param>
	/// <param name="fromPos">基準点（プレイヤー位置）</param>
	/// <param name="basisDirXZ">基準方向（XZ平面。長さは問わない）</param>
	ITarget* Pick(IEnemyTargetProvider* provider, const Vector3& fromPos, const Vector3& basisDirXZ);

	Params& MutableParams() { return params_; }
	const Params& GetParams() const { return params_; }

	///========================
	// 直近の結果（調整時に「なぜ向かないか」を見るため）
	///========================
	bool  WasFound() const { return found_; }
	float GetLastAngleDeg() const { return lastAngleDeg_; }
	float GetLastDistance() const { return lastDistance_; }

private:
	// 方向が決まらないとみなす長さ（真上・真下や0ベクトル対策）
	static constexpr float kDirectionEpsilon_ = 0.0001f;
	// ラジアン→度
	static constexpr float kRadToDeg_ = 57.29578f;

	Params params_{};

	// 毎フレームの確保を避けるための使い回しバッファ
	std::vector<ITarget*> scratch_;

	// 直近の選択結果（デバッグ表示用）
	bool  found_ = false;
	float lastAngleDeg_ = 0.0f;
	float lastDistance_ = 0.0f;
};
