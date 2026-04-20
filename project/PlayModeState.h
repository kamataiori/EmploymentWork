#pragma once

#include <functional>
#include "engine/TimeManager.h"

/// <summary>
/// ゲームの再生/停止状態を管理するクラス
/// 
/// UE5 の Play/Stop ボタンと同じ役割:
///   Play()      : 再生開始 (シーンを最初からやり直し + TimeScale=1)
///   Stop()      : 停止     (TimeScale=0 でゲームロジック停止)
///   IsPlaying() : 現在再生中か
/// 
/// 設計:
///   - 停止の実現方法は "TimeScale を 0 にする" 方式 (Unity/UE5 Editor 流)
///   - Scene::Update() は停止中も常に呼ばれる
///   - dt 依存のゲームロジックは TimeScale=0 で自動停止
///   - 行列計算などの非-dt処理は継続 → 画面が最新の状態で維持される
///   - エディタ系演出は UnscaledDeltaTime を使うことで停止中も動作可能
///   
///   シーンリセットの方法は外部から注入する(onPlayCallback_)。
///   これにより、PlayModeState は SceneManager に直接依存しない。
/// </summary>
class PlayModeState
{
public:
	using Callback = std::function<void()>;

	/// <summary>
	/// Play押下時に追加で呼ばれるコールバックを登録
	/// (典型的には SceneManager で現在のシーンを再初期化する処理)
	/// </summary>
	void SetOnPlayCallback(Callback cb) { onPlayCallback_ = std::move(cb); }

	/// <summary>
	/// Stop押下時に追加で呼ばれるコールバックを登録
	/// </summary>
	void SetOnStopCallback(Callback cb) { onStopCallback_ = std::move(cb); }

	/// <summary>
	/// 再生開始
	///   - TimeScale を playingTimeScale_ (通常1.0) に戻す
	///   - 登録されたコールバックでシーンがリセットされる
	///   - IsPlaying() == true になる
	/// </summary>
	void Play()
	{
		if (isPlaying_) {
			return;
		}

		// --- ゲームロジック進行を再開 ---
		TimeManager::GetInstance()->SetTimeScale(playingTimeScale_);

		// --- シーンリセット等 ---
		if (onPlayCallback_) {
			onPlayCallback_();
		}

		isPlaying_ = true;
	}

	/// <summary>
	/// 停止
	///   - TimeScale を 0 にする (dt=0 で全てのゲームロジックが自動停止)
	///   - Scene::Update 自体は常に呼ばれ続ける
	///   - IsPlaying() == false になる
	/// </summary>
	void Stop()
	{
		if (!isPlaying_) {
			// 未再生状態でも、初期化直後に TimeScale=0 を保証したいので
			// 毎回強制的に 0 にしておく (冪等)
			TimeManager::GetInstance()->SetTimeScale(0.0f);
			return;
		}

		// --- ゲームロジック進行を停止 ---
		TimeManager::GetInstance()->SetTimeScale(0.0f);

		if (onStopCallback_) {
			onStopCallback_();
		}

		isPlaying_ = false;
	}

	/// <summary>
	/// 起動直後に「停止状態」を確定させるための初期化
	/// MyGame::Initialize で呼ぶ想定
	/// </summary>
	void InitializeStopped()
	{
		isPlaying_ = false;
		TimeManager::GetInstance()->SetTimeScale(0.0f);
	}

	/// <summary>再生中かどうか</summary>
	bool IsPlaying() const { return isPlaying_; }

	/// <summary>
	/// 再生中に使う TimeScale (デフォルト 1.0)
	/// スローモー演出などでゲーム全体の速度を落としたい場合に変更する
	/// </summary>
	void SetPlayingTimeScale(float scale)
	{
		if (scale < 0.0f) scale = 0.0f;
		playingTimeScale_ = scale;
		// すでに再生中なら即反映
		if (isPlaying_) {
			TimeManager::GetInstance()->SetTimeScale(playingTimeScale_);
		}
	}

	float GetPlayingTimeScale() const { return playingTimeScale_; }

private:
	// 再生中フラグ
	// 起動直後は停止状態(UE5風)
	bool isPlaying_ = false;

	// 再生中に設定する TimeScale (通常 1.0)
	float playingTimeScale_ = 1.0f;

	// コールバック
	Callback onPlayCallback_;
	Callback onStopCallback_;
};