#pragma once
#include "RenderTarget.h"
#include "OffscreenRendering.h"
#include <memory>
#include <cstdint>
#include <Vector4.h>

class DirectXCommon;

/// <summary>
/// 描画レイヤー（Canvas）。
/// 自前のオフスクリーン RenderTarget（描画先）と、専用のポストエフェクトスタック
/// （OffscreenRendering）を持つ。「このCanvasへ描く → 好きなポストエフェクトを適用
/// → 結果テクスチャ(SRV)を得る」という単位で、複数のCanvasを LayerCompositor が
/// バックバッファへ合成する。
///
/// レイヤーごとに別々のエフェクトを掛けられる（例：パーティクル層だけBloom、
/// オブジェクト層はVignette）。エフェクトの種類は SetPostEffect で動的に切替可能。
///
/// 使い方:
///   canvas.BeginScene(cmd);   // このCanvasへ描画開始（クリア＋バインド）
///   ... 描画コマンド ...
///   canvas.EndScene(cmd);     // 描画終了（SRVとして参照可能に）
///   canvas.Resolve(cmd);      // ポストエフェクト適用 → 結果テクスチャ確定
///   uint32_t srv = canvas.GetResultSrvIndex();  // 合成に使う
/// </summary>
class Canvas {
public:
	void Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height,
		const Vector4& clearColor);

	// このCanvasへの描画を開始（RTをバインドしてクリア）。
	// clearDepth=false のときは深度をクリアせずエンジン深度を共有する
	// （パーティクルをワールド深度で正しく遮蔽させたい場合に使う）。
	void BeginScene(ID3D12GraphicsCommandList* commandList, bool clearDepth = true);
	// このCanvasへの描画を終了（サンプル可能な状態へ遷移）
	void EndScene(ID3D12GraphicsCommandList* commandList);

	// ポストエフェクトを適用し、結果テクスチャを確定する
	void Resolve(ID3D12GraphicsCommandList* commandList);

	// 合成に使う結果テクスチャのSRVインデックス
	uint32_t GetResultSrvIndex() const { return resultSrvIndex_; }

	// このレイヤーに掛けるポストエフェクトの種類を切替える（動的）
	void SetPostEffect(PostEffectType type) { effect_->SetPostEffectType(type); }
	// エフェクトスタック本体（型別setterで細かく調整するため）
	OffscreenRendering* GetEffect() const { return effect_.get(); }

	RenderTarget* GetSceneRenderTarget() const { return sceneRT_.get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	Vector4 clearColor_{};

	std::unique_ptr<RenderTarget> sceneRT_;   // このCanvasへ描画する先
	std::unique_ptr<RenderTarget> outputRT_;  // エフェクト適用後の結果
	std::unique_ptr<OffscreenRendering> effect_;  // このレイヤー専用のポストエフェクトスタック

	uint32_t resultSrvIndex_ = 0;
};
