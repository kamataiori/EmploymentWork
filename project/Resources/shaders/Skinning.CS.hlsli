// 構造体定義
struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Vertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

struct VertexInfluence
{
    float4 weight;
    int4 index;
};

struct SkinningInformation
{
    uint numVertices;
};

//-------各種Buffer定義-------//

// SkinningObject3d.VS.hlslで作ったものと同じPalette
StructuredBuffer<Well> gMatrixPalette : register(t0);
// VertexBufferViewのstream0として利用していた入力頂点
StructuredBuffer<Vertex> gInputVertices : register(t1);
// VertexBufferViewのstream1として利用していた入力インフルエンス
StructuredBuffer<VertexInfluence> gInfluences : register(t2);
// Skinning計算後の頂点データ。SkinnedVertex
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);
// Skinningに関するちょっとした情報
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);


[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    uint32_t vertexIndex = DTid.x;
    if (vertexIndex < gSkinningInformation.numVertices)
    {
        //------Skinning計算------//
        
        // 必要なデータをStructuredBufferから取ってくる
        Vertex input = gInputVertices[vertexIndex];
        VertexInfluence influence = gInfluences[vertexIndex];

        // skinning後の頂点を計算
        Vertex skinned;
        skinned.texcoord = input.texcoord;
        
        // スキニングの計算（重み付き行列変換）
        skinned.position =
            mul(input.position, gMatrixPalette[influence.index.x].skeletonSpaceMatrix) * influence.weight.x +
            mul(input.position, gMatrixPalette[influence.index.y].skeletonSpaceMatrix) * influence.weight.y +
            mul(input.position, gMatrixPalette[influence.index.z].skeletonSpaceMatrix) * influence.weight.z +
            mul(input.position, gMatrixPalette[influence.index.w].skeletonSpaceMatrix) * influence.weight.w;

        skinned.position.w = 1.0f;

        skinned.normal =
            mul(input.normal, (float3x3) gMatrixPalette[influence.index.x].skeletonSpaceInverseTransposeMatrix) * influence.weight.x +
            mul(input.normal, (float3x3) gMatrixPalette[influence.index.y].skeletonSpaceInverseTransposeMatrix) * influence.weight.y +
            mul(input.normal, (float3x3) gMatrixPalette[influence.index.z].skeletonSpaceInverseTransposeMatrix) * influence.weight.z +
            mul(input.normal, (float3x3) gMatrixPalette[influence.index.w].skeletonSpaceInverseTransposeMatrix) * influence.weight.w;

        skinned.normal = normalize(skinned.normal);
        
         // Skinning後の頂点データを書き込む
        gOutputVertices[vertexIndex] = skinned;
        
    }
    
}
