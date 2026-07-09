#pragma once
#include <d3d12.h>
#include <cstdint>

class DirectXCommon;
class RenderTarget;

/// <summary>
/// ポストエフェクト1パスの共通インターフェイス（コンポーネント指向）。
/// 各エフェクト（Vignette / Grayscale / Bloom など）はこれを実装した
/// 独立コンポーネントとなり、PostProcessChain に並べて適用される。
/// </summary>
class IPostEffectPass {
public:
	virtual ~IPostEffectPass() = default;

	/// <summary>
	/// PSO・RootSignature・定数バッファなどの生成
	/// </summary>
	virtual void Initialize(DirectXCommon* dxCommon) = 0;

	/// <summary>
	/// 1パスを適用する。
	/// inputSrvIndex を入力テクスチャとして、現在バインド中のRTVへ
	/// フルスクリーン三角形を描画する。
	/// </summary>
	virtual void Execute(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex) = 0;

	/// <summary>
	/// 最終出力先を伝える（チェーンが最後のパスに対して呼ぶ）。
	/// 内部で複数RTを切り替えるマルチパス効果（Bloom等）が、処理後に
	/// 最終出力先へ描き戻すために使う。output=nullptr はバックバッファ。
	/// 通常のパスは既にバインド済みのRTVへ描くだけなので無視してよい（既定は空）。
	/// </summary>
	virtual void SetFinalOutput(DirectXCommon* /*dxCommon*/, RenderTarget* /*output*/) {}
};
