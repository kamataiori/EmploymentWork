// PostEffectManager.h
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

private:
	PostEffectManager() = default;
	~PostEffectManager() = default;
	PostEffectManager(const PostEffectManager&) = delete;
	PostEffectManager& operator=(const PostEffectManager&) = delete;

private:
	std::unique_ptr<PostEffect> postEffect_ = nullptr;
};
