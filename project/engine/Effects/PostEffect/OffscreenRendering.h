#pragma once
#include "DirectXCommon.h"
#include "PostEffectPass.h"
#include "PostProcessChain.h"
#include "BloomPass.h"
#include <array>
#include <memory>
#include <Vector3.h>
#include <Vector2.h>

class RenderTarget;

enum class PostEffectType {
	Normal,
	Blur5x5,
	Blur3x3,
	GaussianFilter,
	RadialBlur,
	Grayscale,
	Vignette,
	Sepia,
	Random,
	Dissolve,
	Bloom,
	SlashCut,
	// 追加可能
	Count
};

constexpr size_t kPostEffectCount = static_cast<size_t>(PostEffectType::Count);

/// <summary>
/// 組み込みポストエフェクトの登録簿＋単一エフェクト適用の調整役。
/// 各エフェクトは IPostEffectPass コンポーネントとして保持し、
/// SetPostEffectType で PostProcessChain のアクティブパスを差し替える。
/// 公開APIは従来と互換（PostEffectManager 経由の呼び出しを壊さない）。
/// </summary>
class OffscreenRendering
{
public:  // publicメンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(PostEffectType type);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画（従来: シーンRT → 現在のバックバッファ）
	/// </summary>
	void Draw();

	/// <summary>
	/// 任意の入力SRVを入力に、現在アクティブなエフェクトを適用して
	/// output の RenderTarget へ書き出す（Canvasレイヤー用）。
	/// エフェクトが Normal でも単純コピーとして output に転写される。
	/// </summary>
	void ApplyTo(ID3D12GraphicsCommandList* commandList, uint32_t inputSrvIndex,
		RenderTarget* output);

	// シーンテクスチャのSRVインデックス
	uint32_t GetSrvIndex() const { return srvIndex_; }

	/// <summary>
	/// ポストエフェクトの種類を設定
	/// </summary>
	void SetPostEffectType(PostEffectType type);

	/// <summary>
	/// 現在のポストエフェクトの種類を取得
	/// </summary>
	PostEffectType GetPostEffectType() const;

	//--------Vignette--------//
	void VignetteInitialize(float scale, float power, const Vector3& color);
	void SetVignetteScale(float scale);
	void SetVignettePower(float power);
	void SetVignetteColor(const Vector3& color);

	//--------Grayscale--------//
	void GrayscaleInitialize(float strength, const Vector3& weights);
	void SetGrayscaleStrength(float strength);
	void SetGrayscaleWeights(const Vector3& weights);

	//--------Sepia--------//
	void SepiaInitialize(const Vector3& sepiaColor, float strength);
	void SetSepiaColor(const Vector3& sepiaColor);
	void SetSepiaStrength(float strength);

	//--------RadialBlur--------//
	void RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples);
	void SetRadialBlurCenter(const Vector2& center);
	void SetRadialBlurWidth(float blurWidth);
	void SetRadialBlurSamples(int numSamples);

	//--------Random--------//
	void RandomInitialize(float time, float scale, float intensity);
	void SetRandomTime(float time);
	void SetRandomScale(float scale);
	void SetRandomIntensity(float intensity);
	void SetRandomUseImage(bool useImage);

	//--------Dissolve--------//
	void DissolveInitialize(float threshold, float edgeWidth, const Vector3& edgeColor);
	void SetDissolveThreshold(float value);
	void SetDissolveEdgeWidth(float value);
	void SetDissolveEdgeColor(const Vector3& color);
	void SetDissolveTextures(const std::string& scenePath, const std::string& noisePath);

	//--------Bloom--------//
	void SetBloomThreshold(float threshold);
	void SetBloomIntensity(float intensity);
	void SetBloomIterations(int iterations);

	//--------SlashCut--------//
	void SlashCutInitialize(float edge, float hSep, float dSep, float slide);
	void SetSlashCutProgress(float hProgress, float dProgress, float fall);

private:  // privateメンバ関数

	// 組み込みパスを全て生成して登録簿に格納
	void CreatePasses();

private:  // privateメンバ変数

	// DirectXCommon
	DirectXCommon* dxCommon_ = nullptr;

	// シーンテクスチャのSRVインデックス（従来どおり 0）
	uint32_t srvIndex_ = 0;

	// 現在のエフェクトタイプ
	PostEffectType currentType_ = PostEffectType::Normal;

	// 組み込みパスの登録簿（型ごとに1つ所有）
	std::array<std::unique_ptr<IPostEffectPass>, kPostEffectCount> passes_{};

	// アクティブなパス列
	PostProcessChain chain_;

	// setterから型付きでアクセスするための生ポインタ（passes_ 内を指す・非所有）
	VignettePass* vignette_ = nullptr;
	GrayscalePass* grayscale_ = nullptr;
	SepiaPass* sepia_ = nullptr;
	RadialBlurPass* radialBlur_ = nullptr;
	RandomPass* random_ = nullptr;
	DissolvePass* dissolve_ = nullptr;
	BloomPass* bloom_ = nullptr;
	SlashCutPass* slashCut_ = nullptr;
};
