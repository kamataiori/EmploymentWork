#include "Sprite.hlsli"

//ConstantBufferを使って色を指定する
struct Material
{
    float4 color : SV_TARGET0;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
   // output.color = float32_t4(1.0, 1.0, 1.0, 1.0);
    //output.color = gMaterial.color;
    output.color = gMaterial.color * textureColor;
    return output;
}