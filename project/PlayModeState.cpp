#include "PlayModeState.h"

void PlayModeState::InitializePlaying()
{
	// 停止状態のセットアップの逆 = 再生状態としてマーク
	isPlaying_ = true;

	// シーン切替直後と同じように「1フレームだけ強制Update」フラグがあれば
	// true にしておく（設計に応じて）
	// forceUpdateOneFrame_ = true;

	// 起動時は onPlayCallback_ を呼ばない（すでに ChangeScene("TITLE") 済みなので
	// ここで ReloadCurrentScene すると二重初期化になる）
}
