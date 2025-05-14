#include "Fullscreen.hlsli"

cbuffer RadialBlurSettings : register(b0)
{
    float2 center; // ブラーの中心座標（0.0～1.0）
    float blurWidth; // ブラーの拡がり幅（小さいほど狭い）
    int numSamples; // サンプル数（多いほど滑らか、処理は重い）
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャ座標から中心へのベクトルを計算
    float2 direction = input.texcoord - center;

    // サンプル色初期化
    float3 result = float3(0.0, 0.0, 0.0);

    // 複数のオフセット位置をサンプリングして平均化（放射状）
    for (int i = 0; i < numSamples; ++i)
    {
        float rate = blurWidth * (float) i;
        float2 offset = direction * rate;
        result += gTexture.Sample(gSamplerLinear, input.texcoord - offset).rgb;
    }

    result /= numSamples; // サンプル数で平均化

    output.color = float4(result, 1.0f); // アルファは固定
    return output;
}
