#include "Fullscreen.hlsli"

// 方向性ガウシアンブラー（横／縦を direction で切り替えて分離実行する）
cbuffer BloomBlurSettings : register(b0)
{
    float2 direction; // (1,0)=横方向, (0,1)=縦方向
    float2 padding;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 9タップ（中心＋片側4）のガウス重み
static const int kTapCount = 5;
static const float kWeights[5] =
{
    0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f
};

PixelShaderOutput main(VertexShaderOutput input)
{
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float2 texel = float2(rcp(width), rcp(height));
    float2 stepUV = direction * texel;

    float3 result = gTexture.Sample(gSampler, input.texcoord).rgb * kWeights[0];
    for (int i = 1; i < kTapCount; ++i)
    {
        float2 offset = stepUV * float(i);
        result += gTexture.Sample(gSampler, input.texcoord + offset).rgb * kWeights[i];
        result += gTexture.Sample(gSampler, input.texcoord - offset).rgb * kWeights[i];
    }

    PixelShaderOutput output;
    output.color.rgb = result;
    output.color.a = 1.0f;
    return output;
}
