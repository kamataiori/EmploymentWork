#pragma once
#include <memory>
#include "PostEffect.h"

class PostEffectManager {
public:
	static PostEffectManager* GetInstance();

	void Initialize(PostEffectType type);
	void Draw();
	void Finalize();

	void SetType(PostEffectType type);
	PostEffectType GetType() const;

	uint32_t GetSrvIndex() const;
	OffscreenRendering* GetOffscreen();

	/// <summary>
	/// Vignetteのsetter
	/// </summary>
	void VignetteInitialize(float scale, float power, const Vector3& color);
	void SetVignetteScale(float scale);
	void SetVignettePower(float power);
	void SetVignetteColor(const Vector3& color);

	/// <summary>
	/// Grayscaleのsetter
	/// </summary>
	void GrayscaleInitialize(float strength, const Vector3& weights);
	void SetGrayscaleStrength(float strength);
	void SetGrayscaleWeights(const Vector3& weights);

	/// <summary>
	/// Sepiaのsetter
	/// </summary>
	void SepiaInitialize(const Vector3& sepiaColor, float strength);
	void SetSepiaColor(const Vector3& sepiaColor);
	void SetSepiaStrength(float strength);

	/// <summary>
	/// RadialBlurのsetter
	/// </summary>
	void RadialBlurInitialize(const Vector2& center, float blurWidth, int numSamples);
	void SetRadialBlurCenter(const Vector2& center);
	void SetRadialBlurWidth(float blurWidth);
	void SetRadialBlurSamples(int numSamples);

	/// <summary>
	/// Randomのsetter
	/// </summary>
	void RandomInitialize(float time, float scale, float intensity);
	void RandomUpdate(float deltaTime);
	void SetRandomTime(float time);
	void SetRandomScale(float scale);
	void SetRandomIntensity(float intensity);
	void SetRandomUseImage(bool useImage);

	/// <summary>
	/// Dissolveのsetter
	/// </summary>
	void DissolveInitialize(float threshold, float edgeWidth, const Vector3& edgeColor);
	void SetDissolveThreshold(float value);
	void SetDissolveEdgeWidth(float value);
	void SetDissolveEdgeColor(const Vector3& color);
	void SetDissolveTextures(const std::string& scenePath, const std::string& noisePath);


private:
	PostEffectManager() = default;
	~PostEffectManager() = default;
	PostEffectManager(const PostEffectManager&) = delete;
	PostEffectManager& operator=(const PostEffectManager&) = delete;

private:
	std::unique_ptr<PostEffect> postEffect_ = nullptr;

	float randomTime_ = 0.0f;
};
