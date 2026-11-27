#include "Fullscreen.hlsli"

// 明るい部分（しきい値超過分）を抽出するパス
cbuffer BloomExtractSettings : register(b0)
{
    float threshold; // この輝度を超えた分だけをBloom対象として抽出
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

    float3 color = gTexture.Sample(gSampler, input.texcoord).rgb;

    // 輝度を計算（グレースケール重み）
    float luminance = dot(color, float3(0.2125f, 0.7154f, 0.0721f));

    // しきい値を超えた分だけを抽出（元の色味を保ちつつ減衰）
    float contribution = max(luminance - threshold, 0.0f);
    float scale = luminance > 0.0001f ? contribution / luminance : 0.0f;

    output.color.rgb = color * scale;
    output.color.a = 1.0f;
    return output;
}
