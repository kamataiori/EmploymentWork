#include "OffscreenRendering.h"
#include "RenderTarget.h"
#include <WinApp.h>

void OffscreenRendering::Initialize(PostEffectType type)
{
	dxCommon_ = DirectXCommon::GetInstance();

	// シーンテクスチャのSRVインデックス（Step1で分離した RenderTarget から取得）
	if (auto* sceneRT = dxCommon_->GetSceneRenderTarget()) {
		srvIndex_ = sceneRT->GetSrvIndex();
	}

	// 組み込みパスを生成
	CreatePasses();

	// チェーン（中間バッファは画面サイズ）
	chain_.Initialize(dxCommon_, WinApp::kClientWidth, WinApp::kClientHeight);

	// 各エフェクトの初期値（従来と同一）
	VignetteInitialize(16.0f, 0.2f, { 1.0f, 1.0f, 0.0f });
	GrayscaleInitialize(1.0f, { 0.2125f, 0.7154f, 0.0721f });
	SepiaInitialize({ 1.0f, 74.0f / 107.0f, 43.0f / 107.0f }, 0.9f);
	RadialBlurInitialize({ 0.5f, 0.5f }, 0.01f, 10);
	RandomInitialize(1.0f, 100.0f, 0.5f);
	DissolveInitialize(0.3f, 0.3f, { 0.2f, 0.4f, 0.5f });
	// edge:断面発光幅 / hSep:横分離量 / dSep:斜め分離量 / slide:横スライド量（画面に対する割合）
	SlashCutInitialize(0.006f, 0.03f, 0.03f, 0.08f);

	// 初期エフェクトを設定
	SetPostEffectType(type);
}

void OffscreenRendering::CreatePasses()
{
	// Normal（コピー）
	{
		auto pass = std::make_unique<CopyPass>();
		pass->Initialize(dxCommon_);
		passes_[static_cast<size_t>(PostEffectType::Normal)] = std::move(pass);
	}
	// RadialBlur
	{
		auto pass = std::make_unique<RadialBlurPass>();
		pass->Initialize(dxCommon_);
		radialBlur_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::RadialBlur)] = std::move(pass);
	}
	// Grayscale
	{
		auto pass = std::make_unique<GrayscalePass>();
		pass->Initialize(dxCommon_);
		grayscale_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Grayscale)] = std::move(pass);
	}
	// Vignette
	{
		auto pass = std::make_unique<VignettePass>();
		pass->Initialize(dxCommon_);
		vignette_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Vignette)] = std::move(pass);
	}
	// Sepia
	{
		auto pass = std::make_unique<SepiaPass>();
		pass->Initialize(dxCommon_);
		sepia_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Sepia)] = std::move(pass);
	}
	// Random
	{
		auto pass = std::make_unique<RandomPass>();
		pass->Initialize(dxCommon_);
		random_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Random)] = std::move(pass);
	}
	// Dissolve
	{
		auto pass = std::make_unique<DissolvePass>();
		pass->Initialize(dxCommon_);
		dissolve_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Dissolve)] = std::move(pass);
	}
	// Bloom（内部で複数RTを回すマルチパス効果）
	{
		auto pass = std::make_unique<BloomPass>();
		pass->Initialize(dxCommon_);
		bloom_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::Bloom)] = std::move(pass);
	}
	// SlashCut（画面全体を斬撃ラインで分離）
	{
		auto pass = std::make_unique<SlashCutPass>();
		pass->Initialize(dxCommon_);
		slashCut_ = pass.get();
		passes_[static_cast<size_t>(PostEffectType::SlashCut)] = std::move(pass);
	}
}

void OffscreenRendering::Update()
{
}

void OffscreenRendering::Draw()
{
	ID3D12GraphicsCommandList* cmd = dxCommon_->GetCommandList().Get();
	chain_.Execute(cmd, srvIndex_);
}

void OffscreenRendering::ApplyTo(ID3D12GraphicsCommandList* commandList,
	uint32_t inputSrvIndex, RenderTarget* output)
{
	// 入力SRV → output へ現在のアクティブパスを適用（Normalなら単純コピー）
	chain_.Execute(commandList, inputSrvIndex, output);
}

void OffscreenRendering::SetPostEffectType(PostEffectType type)
{
	currentType_ = type;

	// アクティブパスを差し替え（登録のない型は Normal にフォールバック）
	chain_.Clear();
	IPostEffectPass* pass = passes_[static_cast<size_t>(type)].get();
	if (!pass) {
		pass = passes_[static_cast<size_t>(PostEffectType::Normal)].get();
	}
	if (pass) {
		chain_.AddPass(pass);
	}
}

PostEffectType OffscreenRendering::GetPostEffectType() const
{
	return currentType_;
}

//--------Vignette--------//
void OffscreenRendering::VignetteInitialize(float scale, float power, const Vector3& color)
{
	if (vignette_) vignette_->SetParams(scale, power, color);
}
void OffscreenRendering::SetVignetteScale(float scale)
{
	if (vignette_) vignette_->SetScale(scale);
}
void OffscreenRendering::SetVignettePower(float power)
{
	if (vignette_) vignette_->SetPower(power);
}
void OffscreenRendering::SetVignetteColor(const Vector3& color)
{
	if (vignette_) vignette_->SetColor(color);
}

//--------Grayscale--------//
void OffscreenRendering::GrayscaleInitialize(float strength, const Vector3& weights)
{
	if (grayscale_) grayscale_->SetParams(strength, weights);
}
void OffscreenRendering::SetGrayscaleStrength(float strength)
{
	if (grayscale_) grayscale_->SetStrength(strength);
}
void OffscreenRendering::SetGrayscaleWeights(const Vector3& weights)
{
	if (grayscale_) grayscale_->SetWeights(weights);
}

//--------Sepia--------//
void OffscreenRendering::SepiaInitialize(const Vector3& sepiaColor, float strength)
{
	if (sepia_) sepia_->SetParams(sepiaColor, strength);
}
void OffscreenRendering::SetSepiaColor(const Vector3& sepiaColor)
{
	if (sepia_) sepia_->SetColor(sepiaColor);
}
void OffscreenRendering::SetSepiaStrength(float strength)
{
	if (sepia_) sepia_->SetStrength(strength);
}

//--------RadialBlur--------//
void OffscreenRendering::RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples)
{
	if (radialBlur_) radialBlur_->SetParams(center, blurWidth, numSamples);
}
void OffscreenRendering::SetRadialBlurCenter(const Vector2& center)
{
	if (radialBlur_) radialBlur_->SetCenter(center);
}
void OffscreenRendering::SetRadialBlurWidth(float blurWidth)
{
	if (radialBlur_) radialBlur_->SetWidth(blurWidth);
}
void OffscreenRendering::SetRadialBlurSamples(int numSamples)
{
	if (radialBlur_) radialBlur_->SetSamples(numSamples);
}

//--------Random--------//
void OffscreenRendering::RandomInitialize(float time, float scale, float intensity)
{
	if (random_) random_->SetParams(time, scale, intensity);
}
void OffscreenRendering::SetRandomTime(float time)
{
	if (random_) random_->SetTime(time);
}
void OffscreenRendering::SetRandomScale(float scale)
{
	if (random_) random_->SetScale(scale);
}
void OffscreenRendering::SetRandomIntensity(float intensity)
{
	if (random_) random_->SetIntensity(intensity);
}
void OffscreenRendering::SetRandomUseImage(bool useImage)
{
	if (random_) random_->SetUseImage(useImage);
}

//--------Dissolve--------//
void OffscreenRendering::DissolveInitialize(float threshold, float edgeWidth, const Vector3& edgeColor)
{
	if (dissolve_) dissolve_->SetParams(threshold, edgeWidth, edgeColor);
}
void OffscreenRendering::SetDissolveThreshold(float value)
{
	if (dissolve_) dissolve_->SetThreshold(value);
}
void OffscreenRendering::SetDissolveEdgeWidth(float value)
{
	if (dissolve_) dissolve_->SetEdgeWidth(value);
}
void OffscreenRendering::SetDissolveEdgeColor(const Vector3& color)
{
	if (dissolve_) dissolve_->SetEdgeColor(color);
}
void OffscreenRendering::SetDissolveTextures(const std::string& scenePath, const std::string& noisePath)
{
	if (dissolve_) dissolve_->SetTextures(scenePath, noisePath);
}

//--------Bloom--------//
void OffscreenRendering::SetBloomThreshold(float threshold)
{
	if (bloom_) bloom_->SetThreshold(threshold);
}
void OffscreenRendering::SetBloomIntensity(float intensity)
{
	if (bloom_) bloom_->SetIntensity(intensity);
}
void OffscreenRendering::SetBloomIterations(int iterations)
{
	if (bloom_) bloom_->SetIterations(iterations);
}

//--------SlashCut--------//
void OffscreenRendering::SlashCutInitialize(float edge, float hSep, float dSep, float slide)
{
	// 進捗は 0 初期化（切断は演出側から進める）
	if (slashCut_) slashCut_->SetStyle(edge, hSep, dSep, slide);
}
void OffscreenRendering::SetSlashCutProgress(float hProgress, float dProgress, float fall)
{
	if (slashCut_) slashCut_->SetProgress(hProgress, dProgress, fall);
}
