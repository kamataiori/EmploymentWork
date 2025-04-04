#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    // テクスチャサンプリング
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // グレースケール値の計算
    float value = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));

    // グレースケール値を適用
    output.color.rgb = float3(value, value, value);
    // セピア調(回想シーンなどで使える)
    output.color.rgb = value * float3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);

    return output;
}
