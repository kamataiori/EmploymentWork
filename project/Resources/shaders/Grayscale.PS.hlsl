#include "Fullscreen.hlsli"

cbuffer GrayscaleSettings : register(b0)
{
    float strength; // グレースケール強度（0.0 = カラー、1.0 = 完全グレー）
    float3 grayscaleWeights; // RGB変換の重み（例：{0.3, 0.59, 0.11}）
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャから色を取得
    float4 color = gTexture.Sample(gSampler, input.texcoord);

    // 重み付き平均でグレースケール変換
    float luminance = dot(color.rgb, grayscaleWeights);
    float3 gray = float3(luminance, luminance, luminance);

    // グレースケールと元の色をstrengthでブレンド
    output.color = float4(lerp(color.rgb, gray, strength), color.a);
    return output;
}
