#pragma once
#include "IPostEffectPass.h"
#include "RenderTarget.h"
#include <vector>
#include <memory>
#include <cstdint>

class DirectXCommon;

/// <summary>
/// ポストエフェクトのパス列（チェーン）
/// アクティブなパスを順に適用する。パスが1つだけなら現在バインド中の
/// バックバッファへ直接描画（＝従来動作と一致）、複数なら ping-pong
/// 中間バッファを経由して最後のパスをバックバッファへ出力する
/// パスの所有権は持たない（登録簿など外部が所有する）
/// </summary>
class PostProcessChain {
public:
	void Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height);

	// アクティブパス操作（動的に追加・削除できる）
	void AddPass(IPostEffectPass* pass) { passes_.push_back(pass); }
	void Clear() { passes_.clear(); }
	bool IsEmpty() const { return passes_.empty(); }
	size_t GetPassCount() const { return passes_.size(); }

	/// <summary>
	/// チェーンを実行する。
	/// sceneSrvIndex を最初の入力とし、最終結果を finalOutput へ出力する。
	/// finalOutput が nullptr の場合は現在バインド中のバックバッファへ出力する。
	/// finalOutput を渡した場合、呼び出し後その RenderTarget は
	/// RENDER_TARGET 状態のまま（サンプルするなら呼び出し側で遷移する）。
	/// </summary>
	void Execute(ID3D12GraphicsCommandList* commandList, uint32_t sceneSrvIndex,
		RenderTarget* finalOutput = nullptr);

	bool IsPingPongReady() const { return pingPongReady_; }

private:
	// マルチパス用の中間バッファを必要時に生成
	void EnsurePingPong();
	// 指定した中間バッファ／出力先をRTVとしてバインド（ビューポート込み）
	void BindRenderTarget(ID3D12GraphicsCommandList* commandList, RenderTarget* target);

private:
	DirectXCommon* dxCommon_ = nullptr;
	std::vector<IPostEffectPass*> passes_;          // 非所有・順序付き
	std::unique_ptr<RenderTarget> pingPong_[2];     // 中間バッファ（遅延生成）
	bool pingPongReady_ = false;
	uint32_t width_ = 0;
	uint32_t height_ = 0;
};
