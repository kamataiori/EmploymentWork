#pragma once
#include "DirectXCommon.h"
#include "DrawTriangleCommon.h"
#include "Color.h"
#include <vector>
#include "Struct.h"
#include "Camera.h"

class DrawTriangle
{
public:

    struct TransformationMatrix {
        Matrix4x4 WVP;   // World-View-Projection 行列
        Matrix4x4 World; // World 行列
    };

    /// <summary>
    /// インスタンスを取得
    /// </summary>
    static DrawTriangle* GetInstance();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化処理
    /// </summary>
    void Initialize();

    /// <summary>
    /// 三角形の追加
    /// </summary>
    void AddTriangle(const Vector3& p1, const Vector3& p2, const Vector3& p3, Color color,float alpha);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// 三角形の数を取得
    /// </summary>
    size_t GetTriangleCount() const;

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
    DrawTriangle() = default;
    ~DrawTriangle() = default;

    // コピーコンストラクタと代入演算子を削除
    DrawTriangle(const DrawTriangle&) = delete;
    DrawTriangle& operator=(const DrawTriangle&) = delete;

public:
    static DrawTriangle* instance;

private:
    DirectXCommon* dxCommon_ = nullptr;
    DrawTriangleCommon* drawTriangleCommon_ = nullptr;

    // 頂点データとインデックスデータを保持
    std::vector<DrawTriangleCommon::Vertex> vertices_;
    std::vector<uint32_t> indices_;

    // GPUリソースとビュー
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    DrawTriangleCommon::Vertex* vertexData_ = nullptr;
    uint32_t* indexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Camera* camera_ = nullptr;
};
