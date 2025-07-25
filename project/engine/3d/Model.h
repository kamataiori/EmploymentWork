#pragma once
#include <fstream>
#include "ModelCommon.h"
#include "SrvManager.h"
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "StructAnimation.h"
#include "DrawLine.h"


//---前方宣言---//
class ModelCommon;

class Model
{
public:
	//--------構造体--------//

	// 頂点データの拡張
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		//float padding[2];
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
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	// Node構造体
	struct Node {
		QuaternionTransform transform;
		Matrix4x4 localMatrix;  // NodeのTransform
		std::string name;  // Nodeの名前
		std::vector<Node> children;  // 子供のNode
	};

	// Mesh単位でまとめた構造体を追加
	struct  alignas(16) MeshData {
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
		MaterialData material;
		std::map<std::string, JointWeightData> skinClusterData;
	};

	// ModelData構造体を修正
	struct ModelData {
		std::vector<MeshData> meshes; // 複数メッシュに対応
		Node rootNode;
		bool isAnimation;
	};

	struct MeshInstance {
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
		VertexData* vertexData = nullptr;
		Material* materialData = nullptr;
		uint32_t* indexData = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		SkinCluster skinCluster;
		uint32_t textureIndex = 0;
	};

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// Skeltonの更新処理
	/// </summary>
	/// <param name="skeleton"></param>
	void Update(Skeleton& skeleton);

	/// <summary>
	/// SkinClusterの更新処理
	/// </summary>
	void Update(SkinCluster& skinCluster, const Skeleton& skeleton);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// 骨を描画
	/// </summary>
	void DrawSkeleton(const Matrix4x4& worldMatrix);

	/// <summary>
    /// 指定したジョイント名のワールド座標を取得
    /// </summary>
    /// <param name="jointName">取得したいボーン名</param>
    /// <param name="modelWorldMatrix">モデルのワールド行列</param>
    /// <returns>ワールド空間上の位置（存在しない場合は std::nullopt）</returns>
	std::optional<Vector3> GetJointWorldPosition(const std::string& jointName, const Matrix4x4& modelWorldMatrix) const;


	/// <summary>
	/// 頂点データを作成
	/// </summary>
	void CreateVertexData(MeshInstance& instance, const MeshData& data);

	/// <summary>
	/// 解析したデータを使って作成
	/// </summary>
	void CreateIndexResource(MeshInstance& instance, const MeshData& data);

	/// <summary>
	/// マテリアルデータの初期化
	/// </summary>
	void  CreateMaterialData(MeshInstance& instance, const MeshData& data);

	/// <summary>
	/// .mtlファイルの読み取り
	/// </summary>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// Nodeでの階層構造
	/// </summary>
	static Node ReadNode(aiNode* node);

	/// <summary>
	/// .objファイルの読み取り
	/// </summary>
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// Animation解析の関数
	/// </summary>
	AnimationData LoadAnimationFile(const std::string& directoryPath, const std::string& fileName);

	void LoadAllAnimations(const std::string& directoryPath, const std::string& fileName);

	void SetAnimation(const std::string& name);

	/// <summary>
	/// NodeからJointを作る関数
	/// </summary>
	/// <param name="node"></param>
	/// <param name="parent"></param>
	/// <param name="joints"></param>
	/// <returns></returns>
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	/// <summary>
	/// Nodeの階層からSkeletonを作る関数
	/// </summary>
	/// <param name="rootNode"></param>
	/// <returns></returns>
	Skeleton CreateSkeleton(const Node& rootNode);

	/// <summary>
	/// Animationの適用
	/// </summary>
	/// <param name="skeleton"></param>
	/// <param name="animation"></param>
	/// <param name="animationTime"></param>
	void AppAnimation(Skeleton& skeleton, const AnimationData& animation, float animationTime);

	/// <summary>
	/// SkinClusterの生成
	/// </summary>
	SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const MeshData& meshData, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize);

	/// <summary>
	/// ModelDataのGetter
	/// </summary>
	/// <returns></returns>
	const ModelData& GetModelData() const { return modelData; }

	// materialData->colorのゲッター
	const Vector4& GetMaterialColor() const { return material->color; }

	// materialData->colorのセッター
	void SetMaterialColor(const Vector4& color) { material->color = color; }

	// materialData->enableLightingのゲッター
	bool GetEnableLighting() const;

	// materialDataのゲッター
	Model::Material* GetMaterial() const { return material; }

	void SetEnableLighting(bool enable);


private:

	template <typename T>
	T CalculateValue(const std::vector<Keyframe<T>>& keyframes, float time)
	{
		assert(!keyframes.empty());  // キーがないものは返す値がわからないのでダメ
		if (keyframes.size() == 1 || time <= keyframes[0].time)
		{
			// キーが1つか、時刻がキーフレーム前なら最初の値とする
			return keyframes[0].value;
		}

		// 補間のためのループ
		for (size_t index = 0; index < keyframes.size() - 1; ++index) {
			size_t nextIndex = index + 1;
			// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
			if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
				// 範囲内なら補間する
				float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
				if constexpr (std::is_same<T, Vector3>::value) {
					return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
				}
				else if constexpr (std::is_same<T, Quaternion>::value) {
					return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
				}
			}
		}

		// ここまできた場合は一番後の時刻よりも後なので最後の値を返す
		return keyframes.back().value;
	}

private:
	// ModelCommonの初期化
	ModelCommon* modelCommon_ = nullptr;

	// モデル読み込み
	ModelData modelData;

	std::vector<MeshInstance> meshInstances_;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;  // 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;  // マテリアル用の定数バッファ
	// バッファリソース内のデータを指すポインタ
	Material* material = nullptr;
	// バッファリソースの使い道を補完するビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// Animationの時間
	float animationTime = 0.0f;

	AnimationData animation;
	Skeleton skeleton;
	AnimationData* prevAnimation_ = nullptr;
	float blendTime_ = 0.0f;
	float blendDuration_ = 0.4f; // 補間に使う秒数

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	// Viewを作成する
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	uint32_t* mappedIndex = nullptr;

	uint32_t paletteIndex;

	// 
	std::map<std::string, AnimationData> animationMap_;
	AnimationData* currentAnimation_ = nullptr;


};

