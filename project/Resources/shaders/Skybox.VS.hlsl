#include "Skybox.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct TransformationMatrix
{
    float4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // 遠方に表示するため z = w にする (NDC空間で z = 1)
    output.position = mul(input.position, gTransformationMatrix.WVP).xyww;

    // position.xyz をそのまま texcoord に使う（方向ベクトル）
    output.texcoord = input.position.xyz;

    return output;
}
