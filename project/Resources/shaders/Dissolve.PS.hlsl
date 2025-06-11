#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0); // 通常のシーン画像
Texture2D<float> gMaskTexture : register(t1); // マスク用ノイズ画像
SamplerState gSampler : register(s0);

cbuffer DissolveCB : register(b0)
{
    float threshold; // ディゾルブのしきい値
    float edgeWidth; // エッジ幅 (例: 0.03f)
    float3 edgeColor; // エッジ色 (例: float3(1.0, 0.4, 0.3))
};


struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // マスク値を取得
    float mask = gMaskTexture.Sample(gSampler, input.texcoord).r;

    // 通常のDissolve discard
    if (mask < threshold)
    {
        discard;
    }

    // 通常の色を取得
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // エッジ部分の強さをsmoothstepで取得
    float edge = 1.0f - smoothstep(threshold, threshold + edgeWidth, mask);

    // エッジ色を加算
    output.color.rgb += edge * edgeColor;

    return output;

}
