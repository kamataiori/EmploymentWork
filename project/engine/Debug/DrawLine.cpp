#include "DrawLine.h"
#include <cmath>

DrawLine* DrawLine::instance = nullptr;

DrawLine* DrawLine::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new DrawLine;
    }
    return instance;
}

void DrawLine::Finalize()
{
    delete instance;
    instance = nullptr;
}

void DrawLine::Initialize()
{
    // DirectXCommon のインスタンスを取得
    dxCommon_ = DirectXCommon::GetInstance();

    // DrawLineCommon のインスタンスを取得
    drawLineCommon_ = DrawLineCommon::GetInstance();

    // TransformationMatrix 用のバッファを作成
    transformationMatrixResource_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    *transformationMatrixData_ = { MakeIdentity4x4(), MakeIdentity4x4() }; // 初期値は単位行列
}

void DrawLine::AddLine(const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
    Color startColor, Color endColor) {
    // 現在の頂点数を基準にインデックスを設定
    uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());

    // 色を取得してfloat配列に変換
    DirectX::XMFLOAT4 startColorValue = GetColorValue(startColor);
    DirectX::XMFLOAT4 endColorValue = GetColorValue(endColor);

    // 頂点データを追加
    vertices_.push_back({
        {startPoint.x, startPoint.y, startPoint.z}, // 座標
        {startColorValue.x, startColorValue.y, startColorValue.z, startColorValue.w} // 色
        });

    vertices_.push_back({
        {endPoint.x, endPoint.y, endPoint.z}, // 座標
        {endColorValue.x, endColorValue.y, endColorValue.z, endColorValue.w} // 色
        });

    // インデックスデータを追加
    indices_.push_back(baseIndex);
    indices_.push_back(baseIndex + 1);
}

void DrawLine::DrawAABB(const AABB& aabb)
{
    // 8つの頂点を計算
    Vector3 vertices[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z}, // 0: min
        {aabb.max.x, aabb.min.y, aabb.min.z}, // 1: +x
        {aabb.min.x, aabb.max.y, aabb.min.z}, // 2: +y
        {aabb.max.x, aabb.max.y, aabb.min.z}, // 3: +x, +y
        {aabb.min.x, aabb.min.y, aabb.max.z}, // 4: +z
        {aabb.max.x, aabb.min.y, aabb.max.z}, // 5: +x, +z
        {aabb.min.x, aabb.max.y, aabb.max.z}, // 6: +y, +z
        {aabb.max.x, aabb.max.y, aabb.max.z}  // 7: +x, +y, +z
    };

    // 色を取得
    Color color = static_cast<Color>(aabb.color);

    // 各エッジを描画
    AddLine(ToXMFLOAT3(vertices[0]), ToXMFLOAT3(vertices[1]), color, color); // Edge 0-1
    AddLine(ToXMFLOAT3(vertices[1]), ToXMFLOAT3(vertices[3]), color, color); // Edge 1-3
    AddLine(ToXMFLOAT3(vertices[3]), ToXMFLOAT3(vertices[2]), color, color); // Edge 3-2
    AddLine(ToXMFLOAT3(vertices[2]), ToXMFLOAT3(vertices[0]), color, color); // Edge 2-0

    AddLine(ToXMFLOAT3(vertices[4]), ToXMFLOAT3(vertices[5]), color, color); // Edge 4-5
    AddLine(ToXMFLOAT3(vertices[5]), ToXMFLOAT3(vertices[7]), color, color); // Edge 5-7
    AddLine(ToXMFLOAT3(vertices[7]), ToXMFLOAT3(vertices[6]), color, color); // Edge 7-6
    AddLine(ToXMFLOAT3(vertices[6]), ToXMFLOAT3(vertices[4]), color, color); // Edge 6-4

    AddLine(ToXMFLOAT3(vertices[0]), ToXMFLOAT3(vertices[4]), color, color); // Edge 0-4
    AddLine(ToXMFLOAT3(vertices[1]), ToXMFLOAT3(vertices[5]), color, color); // Edge 1-5
    AddLine(ToXMFLOAT3(vertices[2]), ToXMFLOAT3(vertices[6]), color, color); // Edge 2-6
    AddLine(ToXMFLOAT3(vertices[3]), ToXMFLOAT3(vertices[7]), color, color); // Edge 3-7
}

void DrawLine::DrawOBB(const OBB& obb)
{
    // 色設定
    Color lineColor = static_cast<Color>(obb.color);

    // 8つの頂点を計算
    Vector3 corners[8];

    // サイズベクトル
    Vector3 halfX = obb.orientations[0] * obb.size.x;
    Vector3 halfY = obb.orientations[1] * obb.size.y;
    Vector3 halfZ = obb.orientations[2] * obb.size.z;

    // 各頂点を計算
    corners[0] = obb.center - halfX - halfY - halfZ; // 左下手前
    corners[1] = obb.center + halfX - halfY - halfZ; // 右下手前
    corners[2] = obb.center + halfX + halfY - halfZ; // 右上手前
    corners[3] = obb.center - halfX + halfY - halfZ; // 左上手前
    corners[4] = obb.center - halfX - halfY + halfZ; // 左下奥
    corners[5] = obb.center + halfX - halfY + halfZ; // 右下奥
    corners[6] = obb.center + halfX + halfY + halfZ; // 右上奥
    corners[7] = obb.center - halfX + halfY + halfZ; // 左上奥

    // 12本のエッジを描画
    // 手前面
    AddLine(ToXMFLOAT3(corners[0]), ToXMFLOAT3(corners[1]), lineColor, lineColor); // 0-1
    AddLine(ToXMFLOAT3(corners[1]), ToXMFLOAT3(corners[2]), lineColor, lineColor); // 1-2
    AddLine(ToXMFLOAT3(corners[2]), ToXMFLOAT3(corners[3]), lineColor, lineColor); // 2-3
    AddLine(ToXMFLOAT3(corners[3]), ToXMFLOAT3(corners[0]), lineColor, lineColor); // 3-0

    // 奥面
    AddLine(ToXMFLOAT3(corners[4]), ToXMFLOAT3(corners[5]), lineColor, lineColor); // 4-5
    AddLine(ToXMFLOAT3(corners[5]), ToXMFLOAT3(corners[6]), lineColor, lineColor); // 5-6
    AddLine(ToXMFLOAT3(corners[6]), ToXMFLOAT3(corners[7]), lineColor, lineColor); // 6-7
    AddLine(ToXMFLOAT3(corners[7]), ToXMFLOAT3(corners[4]), lineColor, lineColor); // 7-4

    // 側面
    AddLine(ToXMFLOAT3(corners[0]), ToXMFLOAT3(corners[4]), lineColor, lineColor); // 0-4
    AddLine(ToXMFLOAT3(corners[1]), ToXMFLOAT3(corners[5]), lineColor, lineColor); // 1-5
    AddLine(ToXMFLOAT3(corners[2]), ToXMFLOAT3(corners[6]), lineColor, lineColor); // 2-6
    AddLine(ToXMFLOAT3(corners[3]), ToXMFLOAT3(corners[7]), lineColor, lineColor); // 3-7
}

void DrawLine::DrawSphere(const Sphere& sphere, int latDiv, int lonDiv)
{
    const float PI = 3.14159265358979323846f;
    Color color = static_cast<Color>(sphere.color);

    // 緯度方向に分割
    for (int lat = 0; lat <= latDiv; ++lat) {
        float theta = PI * lat / latDiv; // 緯度角
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        // 経度方向に分割
        for (int lon = 0; lon < lonDiv; ++lon) {
            float phi = 2.0f * PI * lon / lonDiv; // 経度角
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            // 現在の点
            Vector3 current = {
                sphere.center.x + sphere.radius * sinTheta * cosPhi,
                sphere.center.y + sphere.radius * cosTheta,
                sphere.center.z + sphere.radius * sinTheta * sinPhi
            };

            // 次の経度の点
            float nextPhi = 2.0f * PI * (lon + 1) / lonDiv;
            Vector3 nextLon = {
                sphere.center.x + sphere.radius * sinTheta * cosf(nextPhi),
                sphere.center.y + sphere.radius * cosTheta,
                sphere.center.z + sphere.radius * sinTheta * sinf(nextPhi)
            };

            // 緯度の線を描画
            AddLine(ToXMFLOAT3(current), ToXMFLOAT3(nextLon), color, color);

            // 次の緯度の点
            if (lat < latDiv) {
                float nextTheta = PI * (lat + 1) / latDiv;
                float sinNextTheta = sinf(nextTheta);
                float cosNextTheta = cosf(nextTheta);

                Vector3 nextLat = {
                    sphere.center.x + sphere.radius * sinNextTheta * cosPhi,
                    sphere.center.y + sphere.radius * cosNextTheta,
                    sphere.center.z + sphere.radius * sinNextTheta * sinPhi
                };

                // 経度の線を描画
                AddLine(ToXMFLOAT3(current), ToXMFLOAT3(nextLat), color, color);
            }
        }
    }
}

void DrawLine::DrawPlane(const Plane& plane)
{
    // 平面を描画するための四角形を生成
    Vector3 u = { 1.0f, 0.0f, 0.0f }; // 基準軸1
    if (fabsf(plane.normal.y) > 0.9f) {
        u = { 0.0f, 0.0f, 1.0f }; // 法線がY軸に近い場合は別の基準軸を使用
    }

    Vector3 v = Cross(plane.normal, u); // 平面上の基準軸1
    v = Normalize(v) * plane.size;
    u = Cross(v, plane.normal); // 平面上の基準軸2
    u = Normalize(u) * plane.size;

    // 平面の中心点
    Vector3 center = plane.normal * plane.distance;

    // 四角形の4つの頂点
    Vector3 corners[4] = {
        center - u - v, // 左下
        center + u - v, // 右下
        center + u + v, // 右上
        center - u + v  // 左上
    };

    // 外枠の線を描画
    Color color = static_cast<Color>(Color::WHITE);
    AddLine(ToXMFLOAT3(corners[0]), ToXMFLOAT3(corners[1]), color, color); // 下辺
    AddLine(ToXMFLOAT3(corners[1]), ToXMFLOAT3(corners[2]), color, color); // 右辺
    AddLine(ToXMFLOAT3(corners[2]), ToXMFLOAT3(corners[3]), color, color); // 上辺
    AddLine(ToXMFLOAT3(corners[3]), ToXMFLOAT3(corners[0]), color, color); // 左辺

    // グリッド線を描画
    for (int i = 1; i < plane.divisions; ++i) {
        float t = static_cast<float>(i) / plane.divisions;

        // 縦線（V軸方向に移動）
        Vector3 startV = Lerp(corners[0], corners[3], t);
        Vector3 endV = Lerp(corners[1], corners[2], t);
        AddLine(ToXMFLOAT3(startV), ToXMFLOAT3(endV), color, color);

        // 横線（U軸方向に移動）
        Vector3 startU = Lerp(corners[0], corners[1], t);
        Vector3 endU = Lerp(corners[3], corners[2], t);
        AddLine(ToXMFLOAT3(startU), ToXMFLOAT3(endU), color, color);
    }
}

void DrawLine::DrawCapsule(const Capsule& capsule)
{
    const float PI = 3.14159265358979323846f;
    Color color = static_cast<Color>(capsule.color);

    // カプセルの軸ベクトル
    Vector3 axis = capsule.end - capsule.start;
    float height = Length(axis); // 軸の長さ
    axis = Normalize(axis);

    // カプセルの開始点と終了点
    Vector3 start = capsule.start;
    Vector3 end = capsule.end;

    // 球の描画 (始点)
    Sphere sphereStart = { start, capsule.radius, capsule.color };
    DrawSphere(sphereStart, capsule.rings, capsule.segments);

    // 球の描画 (終点)
    Sphere sphereEnd = { end, capsule.radius, capsule.color };
    DrawSphere(sphereEnd, capsule.rings, capsule.segments);

    // 円柱部分の描画 (円周を構成する線を描画)
    Vector3 u = { 1.0f, 0.0f, 0.0f }; // 基準軸1
    if (fabsf(axis.y) > 0.9f) {
        u = { 0.0f, 0.0f, 1.0f }; // 軸がY軸に近い場合、別の基準軸を使用
    }

    Vector3 v = Cross(axis, u); // 円周の基準軸1
    v = Normalize(v) * capsule.radius;
    u = Cross(v, axis); // 円周の基準軸2
    u = Normalize(u) * capsule.radius;

    // 円周を構成する線を描画
    for (int i = 0; i < capsule.segments; ++i) {
        float angle1 = 2.0f * PI * i / capsule.segments;
        float angle2 = 2.0f * PI * (i + 1) / capsule.segments;

        Vector3 p1Start = start + u * cosf(angle1) + v * sinf(angle1);
        Vector3 p2Start = start + u * cosf(angle2) + v * sinf(angle2);

        Vector3 p1End = end + u * cosf(angle1) + v * sinf(angle1);
        Vector3 p2End = end + u * cosf(angle2) + v * sinf(angle2);

        // 開始点側の円周
        AddLine(ToXMFLOAT3(p1Start), ToXMFLOAT3(p2Start), color, color);

        // 終了点側の円周
        AddLine(ToXMFLOAT3(p1End), ToXMFLOAT3(p2End), color, color);

        // 開始点と終了点を結ぶ線
        AddLine(ToXMFLOAT3(p1Start), ToXMFLOAT3(p1End), color, color);
    }
}

void DrawLine::DrawCone(const Cone& cone)
{
    const float PI = 3.14159265358979323846f;
    Color lineColor = static_cast<Color>(cone.color);

    // 円錐の軸ベクトル
    Vector3 axis = cone.tip - cone.baseCenter;
    Vector3 axisNorm = Normalize(axis);

    // 基準軸の計算
    Vector3 u = { 1.0f, 0.0f, 0.0f }; // 基準軸1
    if (fabsf(axisNorm.y) > 0.9f) {
        u = { 0.0f, 0.0f, 1.0f }; // 軸がY軸に近い場合、別の基準軸を使用
    }
    Vector3 v = Cross(axisNorm, u); // 円周の基準軸1
    v = Normalize(v) * cone.radius;
    u = Cross(v, axisNorm);         // 円周の基準軸2
    u = Normalize(u) * cone.radius;

    // 底面の頂点を計算
    std::vector<Vector3> baseVertices(cone.segments);
    for (int i = 0; i < cone.segments; ++i) {
        float angle = 2.0f * PI * i / cone.segments;
        baseVertices[i] = cone.baseCenter + u * cosf(angle) + v * sinf(angle);
    }

    // 底面の線を描画
    for (int i = 0; i < cone.segments; ++i) {
        int next = (i + 1) % cone.segments;
        AddLine(ToXMFLOAT3(baseVertices[i]), ToXMFLOAT3(baseVertices[next]), lineColor, lineColor);
    }

    // 頂点から底面への線を描画
    for (int i = 0; i < cone.segments; ++i) {
        AddLine(ToXMFLOAT3(cone.tip), ToXMFLOAT3(baseVertices[i]), lineColor, lineColor);
    }
}

void DrawLine::DrawBone(const std::vector<Vector3>& jointPositions, const std::vector<int>& parentIndices, int color)
{
    for (size_t i = 0; i < jointPositions.size(); ++i) {
        if (parentIndices[i] >= 0) {
            Vector3 start = jointPositions[parentIndices[i]];
            Vector3 end = jointPositions[i];
            AddLine(start, end, Color::WHITE, Color::WHITE);
        }
    }
}

void DrawLine::Update() {
    if (!camera_) return;

    // World-View-Projection 行列を計算
    transformationMatrixData_->World = MakeIdentity4x4(); // 必要に応じて変更
    transformationMatrixData_->WVP = Multiply(transformationMatrixData_->World, camera_->GetViewProjectionMatrix());
}

void DrawLine::Draw() {

    if (vertices_.empty() || !camera_) return;

    if (vertices_.size() > 0)
    {
        const size_t vertexBufferSize = vertices_.size() * sizeof(DrawLineCommon::Vertex);
        const size_t indexBufferSize = indices_.size() * sizeof(uint32_t);

        // 頂点バッファの更新
        vertexResource_ = dxCommon_->CreateBufferResource(vertexBufferSize);
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
        memcpy(vertexData_, vertices_.data(), vertexBufferSize);
        vertexResource_->Unmap(0, nullptr);

        // インデックスバッファの更新
        indexResource_ = dxCommon_->CreateBufferResource(indexBufferSize);
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
        memcpy(indexData_, indices_.data(), indexBufferSize);
        indexResource_->Unmap(0, nullptr);

        // バッファビューを更新
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
        vertexBufferView_.StrideInBytes = sizeof(DrawLineCommon::Vertex);

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        // トポロジの設定
        dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        // 頂点バッファの設定
        dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

        // インデックスバッファの設定
        dxCommon_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

        // TransformationMatrix をシェーダーに設定
        dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, transformationMatrixResource_->GetGPUVirtualAddress());

        // 描画コマンドの発行
        dxCommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
    }
}

size_t DrawLine::GetLineCount() const
{
    return vertices_.size() / 2;
}

// 頂点データとインデックスデータをリセットする
void DrawLine::ResetData() {
    vertices_.clear();
    indices_.clear();
    vertexData_ = nullptr;
    indexData_ = nullptr;
}

