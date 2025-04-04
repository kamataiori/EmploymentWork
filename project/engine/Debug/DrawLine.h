#pragma once
#include "DirectXCommon.h"
#include "DrawLineCommon.h"
#include "Color.h"
#include <vector>
#include "Struct.h"
#include "Camera.h"

class DrawLine
{
public:

	struct TransformationMatrix {
		Matrix4x4 WVP;   // World-View-Projection 行列
		Matrix4x4 World; // World 行列
	};

	/// <summary>
	/// インスタンスを取得
	/// </summary>
	static DrawLine* GetInstance();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize();

	/// <summary>
	/// ラインの追加
	/// </summary>
	void AddLine(const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
		Color startColor, Color endColor);

	/// <summary>
	/// AABBの描画
	/// </summary>
	void DrawAABB(const AABB& aabb);

	/// <summary>
	/// OBBの描画
	/// </summary>
	void DrawOBB(const OBB& obb);

	/// <summary>
	/// 球の描画
	/// </summary>
	void DrawSphere(const Sphere& sphere, int latDiv = 16, int lonDiv = 16);

	/// <summary>
	/// グリッドの描画
	/// </summary>
	/// <param name="plane"></param>
	void DrawPlane(const Plane& plane);

	/// <summary>
	/// カプセルの描画
	/// </summary>
	void DrawCapsule(const Capsule& capsule);

	/// <summary>
	/// 円錐の描画
	/// </summary>
	void DrawCone(const Cone& cone);

	/// <summary>
	/// ボーンの描画
	/// </summary>
	void DrawBone(const std::vector<Vector3>& jointPositions, const std::vector<int>& parentIndices, int color = static_cast<int>(Color::WHITE));

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// ラインの数を取得
	/// </summary>
	size_t GetLineCount() const;


	/// <summary>
    /// 頂点データとインデックスデータをリセットする
    /// </summary>
	void ResetData();

	// カメラを設定する
	void SetCamera(Camera* camera) { camera_ = camera; }

private:

	inline DirectX::XMFLOAT3 ToXMFLOAT3(const Vector3& vec) {
		return DirectX::XMFLOAT3(vec.x, vec.y, vec.z);
	}

public:
	// コンストラクタをプライベートに
	DrawLine() = default;
	~DrawLine() = default;

	// コピーコンストラクタと代入演算子を削除
	DrawLine(const DrawLine&) = delete;
	DrawLine& operator=(const DrawLine&) = delete;

public:

	static DrawLine* instance;

private:
	DirectXCommon* dxCommon_ = nullptr;
	DrawLineCommon* drawLineCommon_ = nullptr;

	// 頂点データとインデックスデータを保持
	std::vector<DrawLineCommon::Vertex> vertices_;
	std::vector<uint32_t> indices_;

	// GPUリソースとビュー
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_;

	DrawLineCommon::Vertex* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_; // GPUリソース
	TransformationMatrix* transformationMatrixData_ = nullptr;           // CPU側データポインタ


	Camera* camera_ = nullptr;
};
