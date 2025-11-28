#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <random>
#include <string>
#include <d3d12.h>
#include <Camera.h>
#include <Model.h>
#include "TextureManager.h"
#include "WinApp.h"
#include <CameraManager.h>
#include "MathFunctions.h"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <json.hpp>

class ParticleManager
{
public:

	// メッシュの選択
	enum class VertexDataType {
		Plane,
		Ring,
		Cylinder,
		// 今後追加予定の形状もここに列挙
	};

	//BlendMode
	enum BlendMode {
		//!< ブレンドなし
		kBlendModeNone,
		//!< 通常αブレンド。デフォルト。Src * SrcA + Dest * (1 - SrcA)
		kBlendModeNormal,
		//!< 加算。Src * SrcA + Dest * 1
		kBlendModeAdd,
		//!< 減算。Dest * 1 - Src * SrcA
		kBlendModeSubtract,
		//!< 乗算。Src * 0 + Dest * Src
		kBlendModeMultiply,
		//!< スクリーン。Src * (1 - Dest) + Dest * 1
		kBlendModeScreen,
		// 利用してはいけない
		kCountOfBlendMode,
	};

	//ParticleのEmitter構造体
	struct Emitter {
		Transform transform;
		uint32_t count;  //発生数
		float frequency;  //発生頻度
		float frequencyTime;  //頻度用時刻
	};

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(VertexDataType type);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// パーティクルグループの生成
	/// </summary>
	void CreateParticleGroup(const std::string name, const std::string textureFilePath, BlendMode blendMode = kBlendModeNormal);

	// カメラの設定
	void SetCamera(Camera* camera) { this->camera_ = camera; }

	// cylinderの反転
	void SetFlipYToGroup(const std::string& groupName, bool flip);


	// JSON に保存するパーティクルプリセット
	struct ParticlePreset {
		std::string name;                     // プリセット名（= JSON ファイル名のベース）
		VertexDataType vertexType = VertexDataType::Plane; // 使用するメッシュ形状（Plane/Ring/Cylinder）

		std::string textureFilePath;         // テクスチャパス (Resources/ からの相対パスを想定)
		BlendMode blendMode = kBlendModeNormal;

		// エミッター系の設定（今はとりあえずここに持たせる）
		uint32_t count = 10;                                                // 発生数
		float frequency = 1.0f;                                             // 発生間隔(秒)
		bool repeat = false;                                                // 繰り返し
		bool useRandomPosition = false;                                     // ランダム発生

		// パーティクル共通パラメータ
		bool flipY = false;                                                 // Cylinder の上下反転など
		float lifeTime = 1.0f;                                              // 基本寿命

		Vector3 initialScale = { 1.0f, 1.0f, 1.0f };
		Vector3 initialRotate = { 0.0f, 0.0f, 0.0f };
		Vector3 initialOffset = { 0.0f, 0.0f, 0.0f };                       // エミッター位置からの相対オフセット

		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };                         // RGBA
		bool useBillboard = true;                                           // trueなら常にカメラ目線
		Vector3 velocity = { 0.0f, 0.0f, 0.0f };                            // velocity
		Vector3 rotationSpeed = { 0.0f, 0.0f, 0.0f };                       // 回転速度(rad/sec)
		Vector3 scaleSpeed = { 0.0f, 0.0f, 0.0f };                          // スケール変化速度(/sec)
		bool useGravity = false;                                            // 重力を使うかどうか

	};

	/// ImGui 上でプリセットを編集＆保存するエディタ
	void DrawImGuiParticlePresetEditor();

	/// プリセットを JSON に保存する（直接呼んでもOK）
	bool SavePresetToJson(const ParticlePreset& preset,
		const std::string& directory = "Resources/Particle");

	/// JSON からプリセットを読み込む（name.json を読む）
	bool LoadPresetFromJson(const std::string& presetName,
		ParticlePreset& outPreset,
		const std::string& directory = "Resources/Particle");

	/// ディレクトリ内のすべてのプリセットを読み込む（今後のため）
	void LoadAllPresets(const std::string& directory = "Resources/Particle");

	/// <summary>
	/// JSON プリセット名からパーティクルを発生させる
	/// </summary>
	/// <param name="presetName">JSON ファイル名と同じプリセット名</param>
	/// <param name="emitterTransform">発生元の Transform（位置/回転/スケール）</param>
	void EmitByPresetName(const std::string& presetName, const Transform& emitterTransform);

	const std::string& GetCurrentEditingPresetName() const { return currentEditingPresetName_; }

	// setter
	void SetCurrentEditingPresetName(const std::string& name) { currentEditingPresetName_ = name; }

private:

	std::unordered_map<std::string, ParticlePreset> presets_;  // name -> プリセット

	// プリセットエディタで最後に触っていたプリセット名
	std::string currentEditingPresetName_;

private:

	/// <summary>
	/// 頂点リソースの生成、バッファービューの作成
	/// </summary>
	void VertexBufferView();

	/// <summary>
	/// 頂点データに書き込む
	/// </summary>
	void CreateVertexData();

	/// <summary>
	/// 頂点データに書き込む
	/// </summary>
	void CreateRingVertexData();

	/// <summary>
	/// 頂点データに書き込む
	/// </summary>
	void CreateCylinderVertexData();

	/// <summary>
	/// マテリアルデータの初期化
	/// </summary>
	void  CreateMaterialData();

	/// <summary>
	/// InstancingMax用のResource
	/// </summary>
	void InstancingMaxResource();

private:

	// 頂点データの拡張
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	// マテリアルを拡張する
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
	};

	// MaterialData構造体
	struct MaterialData {
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};

	// Node構造体
	struct Node {
		Matrix4x4 localMatrix;  // NodeのTransform
		std::string name;  // Nodeの名前
		std::vector<Node> children;  // 子供のNode
	};

	// ModelData構造体
	struct ModelData {
		std::vector<VertexData>vertices;
		MaterialData material;
		Node rootNode;
	};

	//Particle構造体
	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector3   rotationSpeed{ 0.0f, 0.0f, 0.0f };   // 回転速度(rad/秒)
		Vector3   scaleSpeed{ 0.0f, 0.0f, 0.0f };      // スケール変化速度(/秒)
		Vector4   color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float lifeTime;
		float currentTime;
	};

	struct ParticleForGPU
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
		float flipY; // ← 追加（0.0f or 1.0f）
		float padding[3]; // アラインメントのため
	};

	// パーティクルグループ構造体の定義
	struct ParticleGroup {
		// マテリアルデータ (テクスチャファイルパスとテクスチャ用SRVインデックス)
		std::string materialFilePath;
		int srvIndex;

		// パーティクルのリスト (std::list<Particle>型)
		std::list<Particle> particleList;

		// インスタンシングデータ用SRVインデックス
		int instancingSrvIndex;

		// インスタンシングリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;

		// インスタンス数
		UINT instanceCount = 0;

		UINT vertexCount = 32; // ← 追加（描画に使うインスタンスあたりの頂点数）

		// インスタンシングデータを書き込むためのポインタ
		ParticleForGPU* instancingDataPtr = nullptr;

		Vector2 textureLeftTop = { 0.0f, 0.0f }; // テクスチャ左上座標
		Vector2 textureSize = { 0.0f, 0.0f }; // テクスチャサイズを追加

		bool flipY = false; // デフォルトは反転しない

		bool useGravity = false;

	};

	/// <summary>
	/// パーティクル生成器
	/// </summary>
	/// <param name="randomEngine"></param>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);

	/// <summary>
	/// Primitiveパーティクル生成器
	/// </summary>
	/// <param name="randomEngine"></param>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle PrimitiveMakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);

	/// <summary>
	/// ringパーティクル生成器
	/// </summary>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle RingMakeNewParticle(const Vector3& translate);

	/// <summary>
	/// cylinderパーティクル生成器
	/// </summary>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle CylinderMakeNewParticle(const Vector3& translate);

public:

	/// <summary>
	/// エミッター
	/// </summary>
	/// <param name="name"></param>
	/// <param name="position"></param>
	/// <param name="count"></param>
	void Emit(const std::string& name, const Transform& transform, uint32_t count, bool useRandomPosition);

	void PrimitiveEmit(const std::string name, const Transform& transform, uint32_t count);

	void RingEmit(const std::string& name, const Transform& transform, uint32_t count);

	void CylinderEmit(const std::string& name, const Transform& transform, uint32_t count);

	// スケール
	void SetScaleToGroup(const std::string& groupName, const Vector3& scale);

	// 回転
	void SetRotationToGroup(const std::string& groupName, const Vector3& rotation);

	// 位置
	void SetTranslationToGroup(const std::string& groupName, const Vector3& translation);

	// 速度
	void SetVelocityToGroup(const std::string& groupName, const Vector3& velocity);

	// 回転速度
	void SetRotationSpeedToGroup(const std::string& groupName, const Vector3& rotationSpeed);

	// スケール速度
	void SetScaleSpeedToGroup(const std::string& groupName, const Vector3& scaleSpeed);

	// 色
	void SetColorToGroup(const std::string& groupName, const Vector4& color);

	// 寿命（LifeTime）を一括設定
	void SetLifeTimeToGroup(const std::string& groupName, float lifeTime);

	// 重力
	void SetGravityToGroup(const std::string& groupName, bool useGravity);

	// 初期時間（CurrentTime）を一括設定
	void SetCurrentTimeToGroup(const std::string& groupName, float currentTime);

	void DeleteParticleGroup(const std::string& groupName);

	void SetUseBillboard(bool use) { usebillboardMatrix = use; }


private:

	// 複数のパーティクルグループを保持
	std::unordered_map<std::string, ParticleGroup> particleGroups;

	// モデル読み込み
	ModelData modelData;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;  // 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;  // マテリアル用の定数バッファ
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	Material* materialData = nullptr;
	// バッファリソースの使い道を補完するビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;

	// 乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine;

	// パーティクルの最大出力数
	const uint32_t kNumMaxInstance = 10000;
	//とりあえず60fps固定してあるが、実時間を計測して可変fpsで動かせるようにしておくとなおよい
	const float kDeltaTime = 1.0f / 60.0f;

private:
	static const int kWindowWidth = 1280;
	static const int kWindowHeight = 720;

	bool usebillboardMatrix = true;

	// Cameraの初期化
	Camera* camera_ = nullptr;
	//常にカメラ目線
	Transform cameraTransform{ {1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f},{0.0f,23.0f,10.0f}};

	Matrix4x4 worldviewProjectionMatrix;

	//テクスチャ切り出しサイズ
	Vector2 textureSize = { 0.0f,0.0f };
	std::string FilePath = {};
	//サイズ
	Vector2 size = { 640.0f,360.0f };

	bool ringSamplerAdd = false;

private:

	//テクスチャサイズイメージ合わせ
	void AdjustTextureSize(ParticleGroup& group, const std::string& textureFilePath);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void RootSignature();

	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	void GraphicsPipelineState(BlendMode blendMode);

	/// <summary>
	/// InputLayoutの設定
	/// </summary>
	void InputLayout();

	/// <summary>
	/// BlendStateの設定
	/// </summary>
	void BlendState(BlendMode blendMode);

	// メンバ変数にブレンドモードを追加
	BlendMode blendMode_; // 現在のブレンドモード
	// ブレンドモードごとにPSOをキャッシュするためのマップ
	std::unordered_map<BlendMode, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateCache_;

	/// <summary>
	/// RasterizerStateの設定
	/// </summary>
	void RasterizerState();

	/// <summary>
	/// DepthStencilStateの設定
	/// </summary>
	void DepthStencilState();

	/// <summary>
	/// PSOの生成
	/// </summary>
	//void PSO();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO();

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
	// Modelの初期化
	Model* model_ = nullptr;
   
	//--------RootSignature部分--------//

	//DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing_[1] = {};

	//RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
	D3D12_ROOT_PARAMETER rootParameters_[7] = {};

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	//--------PSO部分--------//

	//PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//graphicsPipelineStateの生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;

	//Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_{};

	//デフォルトカメラ
	Camera* defaultCamera = nullptr;


	//--------InputLayout部分--------//

	//InputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{};
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

	//--------BlendState部分--------//

	//BlendState
	D3D12_BLEND_DESC blendDesc_{};

	//--------RasterizerState部分--------//

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc_{};

	//--------DepthStencilState部分--------//

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};
};