#include "Object3d.hlsli"

//float4 main( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

//struct VertexShaderOutput
//{
//	float32_t4 position : SV_POSITION;
//};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

//struct VertexShaderOutput
//{
//    float4 position : SV_POSITION;
//    float2 texcoord : TEXCOORD0;
//    float3 normal : NORMAL0;
//};


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    //output.position = input.position;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    
    //////=========法線の座標系を変換してPixelShaderに送る=========////

    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;

    return output;
}




//float4 main(float4 pos : POSITION) : SV_POSITION
//{
//    return pos;
//}

//struct TransformationMatrix
//{
//    float4 WVP;
//};
//ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

//struct VertexShaderOutput
//{
//    float4 position : SV_POSITION;
//};

//struct VertexShaderInput
//{
//    float4 position : POSITION0;
//};

//VertexShaderOutput main(VertexShaderInput input)
//{
//    VertexShaderOutput output;
//    //output.position = input.position;
//    output.position = mul(input.position, gTransformationMatrix.WVP);
//    return output;
//}