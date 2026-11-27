#pragma once
#include "PostEffectPass.h"
#include <cstdint>

class DirectXCommon;

/// <summary>
/// 複数のCanvasの結果テクスチャを、現在バインド中のバックバッファへ
/// 順に合成する。不透明コピー（下地）とアルファ合成（上乗せ）を選べる。
/// </summary>
class LayerCompositor {
public:
	enum class BlendMode {
		Opaque,    // 不透明で上書き（最下レイヤー用）
		Alpha,     // アルファ合成で重ねる（上レイヤー用）
		Additive,  // 加算合成で重ねる（アルファ未書き込みのパーティクル層向け）
	};

	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 指定SRVのテクスチャを、現在バインド中のRTVへフルスクリーン合成する。
	/// </summary>
	void Composite(ID3D12GraphicsCommandList* commandList, uint32_t srvIndex, BlendMode mode);

private:
	DirectXCommon* dxCommon_ = nullptr;
	CopyPass opaquePass_;               // 不透明コピー
	AlphaBlendCopyPass alphaPass_;      // アルファ合成
	AdditiveBlendCopyPass additivePass_; // 加算合成
};
