#include "Fullscreen.hlsli"

// ぼかしたBloomテクスチャを intensity 倍して出力する。
// PSO側で加算ブレンドを設定し、元画像の上に加算合成する。
cbuffer BloomCompositeSettings : register(b0)
{
    float intensity; // Bloomの強さ
    float3 padding;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float3 bloom = gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.rgb = bloom * intensity;
    output.color.a = 1.0f;
    return output;
}
