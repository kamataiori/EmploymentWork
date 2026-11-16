#pragma once
#include "InputAction.h"

/// <summary>
/// シーン単位の入力をまとめるクラスの基底。
///
/// 目的:
///   キーボード / マウス / ゲームパッドという“デバイスの違い”をシーン入力層で吸収し、
///   シーンやキャラには moveX / pause / attack といった“ゲーム上の意味”だけを渡す。
///   (Player が移動入力を直接読み、Scene がポーズ入力を直接読む…という散らばりを無くす)
///
/// この基底が持つのは、どのシーン入力にも共通する2つだけ:
///   1. InputReader を1つ持つ (デバイスの読み取り口)
///   2. 「毎フレーム Update() を1回呼ぶ → あとは Getter を見るだけ」という契約
///
/// 操作そのもの (decide / pause / attack …) は基底に置かない。
///   - タイトルに pause は無く、ゲームに decide は無い。
///     基底へ積むと使わない操作を継承してしまうため。
///   - 各シーン入力が ButtonAction / AxisInput を“必要な数だけ持つ”(コンポーネント)。
///     共通化は「継承」ではなく「同じ部品を使い回す」で達成する。
///
/// 派生の例 (TitleSceneInput / GameSceneInput):
///   void Update() override {
///       moveX_.Update(Reader());
///       pause_.Update(Reader());
///   }
/// </summary>
class SceneInputBase
{
public:
	/// <param name="input">入力デバイス。所有しない (nullptr 可: 常に無入力扱い)</param>
	explicit SceneInputBase(Input* input) : reader_(input) {}
	virtual ~SceneInputBase() = default;

	SceneInputBase(const SceneInputBase&) = delete;
	SceneInputBase& operator=(const SceneInputBase&) = delete;

	/// <summary>
	/// 今フレームの入力を取る。シーンの Update の先頭で1回だけ呼ぶ。
	/// (同じフレーム内では Getter を何度見ても同じ値になる)
	/// </summary>
	virtual void Update() = 0;

	/// <summary>パッドが繋がっているか (操作ヒントの切替やデバッグ表示用)</summary>
	bool IsPadConnected() const { return reader_.IsPadConnected(); }

protected:
	/// <summary>デバイスの読み取り口。派生は各部品の Update() にこれを渡す</summary>
	const InputReader& Reader() const { return reader_; }

private:
	InputReader reader_;
};
