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

class ParticleManager
{
public:
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
    void Initialize();

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
	void CreateParticleGroup(const std::string name, const std::string textureFilePath, BlendMode blendMode = kBlendModeNormal, const Vector2& customSize = { 0.0f, 0.0f });

	// カメラの設定
	void SetCamera(Camera* camera) { this->camera_ = camera; }

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
		Vector4 color;
		float lifeTime;
		float currentTime;
	};

	struct ParticleForGPU
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
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

		// インスタンシングデータを書き込むためのポインタ
		ParticleForGPU* instancingDataPtr = nullptr;

		Vector2 textureLeftTop = { 0.0f, 0.0f }; // テクスチャ左上座標
		Vector2 textureSize = { 0.0f, 0.0f }; // テクスチャサイズを追加
	};

	/// <summary>
	/// パーティクル生成器
	/// </summary>
	/// <param name="randomEngine"></param>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);

public:

	/// <summary>
	/// エミッター
	/// </summary>
	/// <param name="name"></param>
	/// <param name="position"></param>
	/// <param name="count"></param>
	void Emit(const std::string name, const Vector3& position, uint32_t count);

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
	const uint32_t kNumMaxInstance = 128;
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