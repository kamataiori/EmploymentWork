#pragma once
#include "SceneInputBase.h"
#include "IMenuInput.h"

/// <summary>
/// メニュー操作一式（選択の上下・決定・キャンセル・ポインタ）。
///
/// タイトル／メニューシーンはこれをそのままシーン入力として持ち、
/// ゲームプレイ中のポーズ画面へは GameSceneInput がこれを1つ持って貸し出す。
/// こうして「決定は Space / パッドA」といった割り当てを1箇所だけにしている。
///
/// 選択の上下を「軸」と「押した瞬間」の両方で出しているのは、用途が違うため:
///   ・軸 (GetMoveX / GetMoveY) … 傾き具合に意味がある使い方（演出やアナログ移動）
///   ・押した瞬間 (IsUpTriggered / IsDownTriggered) … 項目を1つずつ送る使い方
///
/// シーン固有の操作が増えたときは、このクラスを継承せず
/// 「シーンの入力クラスがこれを1つ持つ」形にする（GameSceneInput と同じ作り）。
///
/// 置き場所:
///   「どのキーに何を割り当てるか」はゲーム側の判断なので application 側に置く。
///   engine 側には仕組み（InputReader / ButtonAction / AxisInput / SceneInputBase）と、
///   UI要素が読むための口（IMenuInput）だけを置く。
/// </summary>
class MenuInput : public SceneInputBase, public IMenuInput
{
public:
	explicit MenuInput(Input* input);

	/// <summary>今フレームの入力を取る（シーンの Update 先頭で1回）</summary>
	void Update() override;

	///========================
	// 軸での取得
	///========================

	/// 横軸 (-1:左 〜 +1:右)
	float GetMoveX() const { return moveX_.GetValue(); }
	/// 縦軸 (-1:下 〜 +1:上)
	float GetMoveY() const { return moveY_.GetValue(); }

	///========================
	// メニュー操作（IMenuInput）
	///========================

	bool IsUpTriggered() const override { return up_.IsTriggered(); }
	bool IsDownTriggered() const override { return down_.IsTriggered(); }
	bool IsDecideTriggered() const override { return decide_.IsTriggered(); }
	bool IsCancelTriggered() const override { return cancel_.IsTriggered(); }
	bool IsPointerClickTriggered() const override { return pointerClick_.IsTriggered(); }
	float GetPointerX() const override { return pointerX_; }
	float GetPointerY() const override { return pointerY_; }

private:
	// 選択の移動（軸としても、1つ送りとしても使えるようにしてある）
	AxisInput moveX_;
	AxisInput moveY_;
	ButtonAction up_;
	ButtonAction down_;

	// 決定・キャンセル
	ButtonAction decide_;
	ButtonAction cancel_;

	// マウスでの指し・クリック
	ButtonAction pointerClick_;
	float pointerX_ = 0.0f;
	float pointerY_ = 0.0f;
};
