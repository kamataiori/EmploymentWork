#pragma once
#include <Vector3.h>
#include <Quaternion.h>
#include <vector>
#include <map>
#include <string>
#include <span>
#include <array>
#include <Matrix4x4.h>

// Keyframe構造体
template <typename tValue>
struct Keyframe {
	float time;
	tValue value;
};
using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

// NodeのAnimation構造体
template<typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};

// Animationを表現する

struct AnimationData {
	float duration;  // アニメーション全体の尺
	// NodeAnimationの集合。Node名でひけるようにしておく
	std::map<std::string, NodeAnimation> NodeAnimations;
};

//=====EulerTransform=========

struct EulerTransform {
	Vector3 scale;
	Vector3 rotate;  // Eulerでの回転
	Vector3 translate;
};

struct QuaternionTransform {
	Vector3 scale;
	Quaternion rotate;
	Vector3 translate;
};

struct Joint {
	QuaternionTransform transform;  // Transform情報
	Matrix4x4 localMatrix;  // localMatrix
	Matrix4x4 skeletonSpaceMatrix;  // skeletonSpaceでの変換行列
	std::string name;  // 名前
	std::vector<int32_t> children;  // 子JointのIndexのリスト。いなければ空き
	int32_t index; // 自身のIndex
	std::optional<int32_t> parent;  // 親JointのIndex。いなければnull
};

struct Skeleton {
	int32_t root;  // RootJointのIndex
	std::map<std::string, int32_t> jointMap;  // Joint名とIndexとの辞書
	std::vector<Joint> joints;  // 所属しているジョイント
};

// VertexWeightData構造体
struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

// JointWeightData構造体
struct JointWeightData {
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

//インフルエンス
const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices;
};

// マトリックスパレット
struct WellForGPU {
	Matrix4x4 skeletonSpaceMatrix; // 位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence> mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};