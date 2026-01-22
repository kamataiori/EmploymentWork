#pragma once
#include <string>

class TutorialController;

// チュートリアル状態の基底クラス
// Enemy の BaseEnemyState と同じ役割
// すべてのチュートリアル状態はこれを継承する
// 具体的な処理は派生クラス側で実装する

class BaseTutorialState {
public:

	//name : デバッグ用の状態名
	//ctx  : この状態を管理している TutorialController
	BaseTutorialState(const std::string& name, TutorialController* ctx)
		: name_(name), ctx_(ctx) {
	}

	virtual ~BaseTutorialState() = default;


	//毎フレーム呼ばれる更新処理
	//各状態固有の条件判定・カウント処理などを書く

	virtual void Update(float dt) = 0;


	//状態に入った瞬間に一度だけ呼ばれる
	//ガイド表示・カウンタ初期化などに使用

	virtual void Enter() {}


	//状態を抜けるときに呼ばれる
	//UIの後始末などに使用（任意）

	virtual void Exit() {}

	// デバッグ用（現在の状態名を取得）
	virtual const char* Name() const { return name_.c_str(); }

protected:

	// 状態名
	std::string name_;
	// 管理元コントローラ
	TutorialController* ctx_ = nullptr;
};
