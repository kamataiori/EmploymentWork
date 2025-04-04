#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // 16の値を小さくすればするほど暗い範囲が中心よりに狭くなる
    float vignette = correct.x * correct.y * 32.0f;
    // 0.8を変えることで丸い形が濃くなったり薄くなったりする
    vignette = saturate(pow(vignette, 0.1f));

    // 黄色のRGB値
    float3 vignetteColor = float3(1.0f, 1.0f, 0.0f);
    
    // vignetteの影響を受ける部分を黄色にブレンド
    output.color.rgb = lerp(vignetteColor, output.color.rgb, vignette);
    
    return output;
}
