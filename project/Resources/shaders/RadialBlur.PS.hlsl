#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    // 中心点 ここを基準に放射状にブラーがかかる
    const float32_t2 kCenter = float32_t2(0.5f, 0.5f);

    // サンプリング数。多いほど滑らかだが重い
    const int32_t kNumSamples = 10;

    // ぼかしの幅。大きいほど広がる
    const float32_t kBlurWidth = 0.01f;

    // 現在のUVに対しての方向を計算
    float32_t2 direction = input.texcoord - kCenter;

    // 出力色を初期化
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);

    // サンプリング
    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        // 現在のUV座標から放射状にサンプリング点を進めながらサンプリングしていく
        float32_t2 texcoord = input.texcoord + direction * kBlurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSamplerLinear, texcoord).rgb;
    }

    // 平均化する
    outputColor.rgb *= rcp(kNumSamples);

    // 出力
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
