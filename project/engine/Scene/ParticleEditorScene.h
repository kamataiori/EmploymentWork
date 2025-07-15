#pragma once
#include "BaseScene.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Sprite.h"
#include "Object3d.h"
#include "SkyBox.h"

inline constexpr const char* kWindowName_Particle = "Particle Control";

class ParticleEditorScene : public BaseScene
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

	void Explosion();

private:
	std::unique_ptr<SkyBox> skybox = std::make_unique<SkyBox>();

	std::unique_ptr<Camera> camera = std::make_unique<Camera>();  // 専用カメラ
	std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>();  // パーティクル管理
	std::unique_ptr<ParticleEmitter> emitter = nullptr;  // エミッター

	// 初期設定を保持してImGuiで操作できるようにする
	Transform emitterTransform = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	EmitterConfig config = { ShapeType::Plane, 200, 1.0f, false };
	std::string groupName = "editor";
	Vector4 particleColor = { 1.0f, 0.5f, 0.0f, 1.0f };  // 初期は白（完全不透明）
	Vector3 particleVelocity = { 0.0f, 0.0f, 0.0f };  // 初期速度
	float particleLifeTime = 1.5f;  // 寿命
	float particleStartTime = 0.0f; // 開始時間
	bool useRandomPosition = true;  // ランダム位置を使うか
	bool useBillboard = true;

	int blendModeIndex = 2;  // kBlendModeAdd に対応（仮にAddから開始）

	std::vector<std::string> textureFiles = {
	"Resources/circle.png",
	"Resources/circle2.png",
	"Resources/gradationLine.png",
	"Resources/image.png",
	"Resources/monsterBall.png",
	"Resources/particleTest.png",
	"Resources/uvChecker.png",
	"Resources/smoke.png",
	"Resources/fire.png",
	};
	int selectedTextureIndex = 0;

	std::unique_ptr<ParticleManager> planeParticle2 = std::make_unique<ParticleManager>();
	std::unique_ptr<ParticleEmitter> emitter2 = nullptr;
	std::string groupName2 = "editor2";


	std::unique_ptr<ParticleManager> ringParticle = std::make_unique<ParticleManager>();  // 爆発リング専用
	std::vector<std::unique_ptr<ParticleEmitter>> ringExplosions;  // 爆発用の複数リング
	std::string explosionGroupName = "ring";  // リング用パーティクルグループ名

	
	std::unique_ptr<ParticleManager> primitiveParticle = std::make_unique<ParticleManager>();
	std::vector<std::unique_ptr<ParticleEmitter>> primitiveEmitters;

	bool explosionEffectStart = false;

};

