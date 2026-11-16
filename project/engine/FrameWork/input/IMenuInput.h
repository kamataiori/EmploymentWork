#pragma once

//======================================================
// IMenuInput
//------------------------------------------------------
// メニュー（タイトル・ポーズ画面など）を操作する入力の読み取り口。
//
// UI要素（engine/UI）が読む側になるので、口もエンジン側に置く。
// 実装は MenuInput（同じフォルダ）で、シーンが持ってUI要素へ貸し出す。
//
// 「選択を動かす」「決定する」といったメニュー操作は、
// タイトルでもポーズ画面でも同じなので、割り当て表を1つにまとめるためのもの。
//======================================================
class IMenuInput
{
public:
	virtual ~IMenuInput() = default;

	/// 選択を上/下へ動かした瞬間
	virtual bool IsUpTriggered() const = 0;
	virtual bool IsDownTriggered() const = 0;

	/// 決定した瞬間（キーボード／パッド）
	virtual bool IsDecideTriggered() const = 0;

	/// キャンセルした瞬間
	virtual bool IsCancelTriggered() const = 0;

	/// ポインタ（マウス）でクリックした瞬間
	virtual bool IsPointerClickTriggered() const = 0;

	/// ポインタの位置[px]（クライアント座標。項目の当たり判定に使う）
	virtual float GetPointerX() const = 0;
	virtual float GetPointerY() const = 0;
};
