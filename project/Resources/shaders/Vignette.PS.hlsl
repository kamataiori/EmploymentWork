#include "Fullscreen.hlsli"

cbuffer VignetteSettings : register(b0)
{
    float vignetteScale; // 値を小さくすればするほど暗い範囲が中心よりに狭くなる
    float vignettePower; // 変えることで丸い形が濃くなったり薄くなったりする
    float3 vignetteColor;
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
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // 周囲をθに、中心になるほど明るくなるように計算で調整
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // correctだけで計算すると中心の最大値が0.0625で暗すぎるのでScaleで調整。この例では16倍して1にしている
    // 16の値を小さくすればするほど暗い範囲が中心よりに狭くなる
    float vignette = correct.x * correct.y * vignetteScale;
    // とりあえず0.8乗でそれっぽくしてみた
    // 0.8を変えることで丸い形が濃くなったり薄くなったりする
    vignette = saturate(pow(vignette, vignettePower));
    // 係数として乗算
   // vignetteの影響を受ける部分をブレンド
    output.color.rgb = lerp(vignetteColor, output.color.rgb, vignette);
    
    return output;
}