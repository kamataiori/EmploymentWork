#include "Skybox.hlsli"

TextureCube<float32_t4> gTexture : register(t0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

//struct Material
//{
//    float4 color : SV_TARGET0;
//    int enableLighting;
//    float4x4 uvTransform;
//    float shininess;
//};

//ConstantBuffer<Material> gMaterial : register(b1);


SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    output.color = textureColor /** gMaterial.color*/;

    return output;
}
