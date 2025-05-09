#include "Fullscreen.hlsli"

cbuffer SepiaSettings : register(b0)
{
    float3 sepiaColor; // セピアカラー補正（R,G,B）
    float sepiaStrength; // セピア強度（0 = グレー、1 = セピア）
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャカラーを取得
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    // グレースケール化（NTSC係数）
    float luminance = dot(texColor.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    float3 gray = float3(luminance, luminance, luminance);

    // セピアカラーを乗算
    float3 sepia = gray * sepiaColor;

    // strength に応じてグレースケールとセピアを線形補間
    output.color.rgb = lerp(gray, sepia, sepiaStrength);
    output.color.a = texColor.a;

    return output;
}
