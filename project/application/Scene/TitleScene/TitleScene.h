#pragma once
#include "BaseScene.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "DrawLine.h"
#include "DrawTriangle.h"
#include "Sprite.h"
#include "Fade.h"
#include "SkyBox.h"

// 簡単な補間とイージング
static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline Vector2 LerpVec2(const Vector2& a, const Vector2& b, float t) {
	return { Lerp(a.x,b.x,t), Lerp(a.y,b.y,t) };
}
// ふわっと止まる EaseOutBack
static inline float EaseOutBack(float t) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}


// TitleScene.h の private より上などに（クラス外）
inline constexpr const char* kWindowName_ParticleControl = "Particle Control";
inline constexpr const char* kWindowName_AABBControl = "AABB Control";
inline constexpr const char* kWindowName_OBBControl = "OBB Control";
inline constexpr const char* kWindowName_SphereControl = "Sphere Control";
inline constexpr const char* kWindowName_DebugInfo = "Debug Information";
inline constexpr const char* kWindowName_TriangleControl = "Triangle Control";

class TitleScene : public BaseScene
{
public:
	//------メンバ関数------

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 背景描画
	/// </summary>
	void BackGroundDraw() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 前景描画
	/// </summary>
	void ForeGroundDraw() override;

	void Debug() override;

private:

	// 3Dオブジェクトの初期化
	std::unique_ptr<Object3d> plane = nullptr;
	std::unique_ptr<Object3d> animationCube = nullptr;
	std::unique_ptr<Object3d> sneak = nullptr;

	//3Dカメラの初期化
	std::unique_ptr<Camera> camera1 = std::make_unique<Camera>();

	std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> emitters;

	std::unique_ptr<ParticleManager> primitiveParticle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> primitiveEmitters;

	std::unique_ptr<ParticleManager> ringParticle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> ringEmitters;

	std::unique_ptr<ParticleManager> cyrinderParticle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> cyrinderEmitters;

	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();

	AABB aabb;
	Sphere sphere;
	Plane ground;
	Capsule capsule;
	OBB obb;

	DrawTriangle* drawTriangle_ = nullptr;
	// 追加するメンバ変数
	Vector3 triangleP1 = { -1.0f, 0.0f, 0.0f };
	Vector3 triangleP2 = { 1.0f, 0.0f, 0.0f };
	Vector3 triangleP3 = { 0.0f, 1.0f, 0.0f };
	Color triangleColor = Color::BLUE;
	// 透過度（0.0f = 完全透明, 1.0f = 不透明）
	float triangleAlpha = 0.3f;


	// 最後の private: 内などに追加
	bool changeSpeed_ = false;

	std::unique_ptr<Fade> fade_ = nullptr;
	std::string nextSceneName_ = "";

	float sliderValue = 0.0f;
	bool isDissolve = false;

	std::unique_ptr<SceneController> sceneController_;

	std::unique_ptr<Sprite> title = std::make_unique<Sprite>();

	struct LetterAnim {
		std::unique_ptr<Sprite> sp;
		Vector2 start;
		Vector2 goal;
		float   t = 0.0f;  // 進捗 0→1
		float   delay = 0.0f;  // 開始遅延(秒)
		float   duration = 16.75f; // 到達時間(秒)
	};

	std::vector<LetterAnim> titleLetters_;   // 「タ」「イ」「ト」「ル」
	std::vector<LetterAnim> spaceLetters_;   // 「s」「p」「a」「c」「e」
	float animClock_ = 0.0f;                 // アニメ用経過時間

	// ---- タイトル用（タ・イ・ト・ル） ----
	std::vector<std::unique_ptr<Sprite>> temp;
	

	// ---- space 用（s p a c e）----
	std::vector<std::unique_ptr<Sprite>> temp2;

	std::unique_ptr<Object3d> sky;
	
};