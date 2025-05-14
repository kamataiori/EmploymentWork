// Random.PS.hlsl
#include "Fullscreen.hlsli"

// ===== 定数バッファ：C++側から制御 =====
cbuffer RandomNoiseSettings : register(b0)
{
    float time; // 時間シード：フレームごとに加算
    float scale; // ノイズの細かさ（座標に掛ける倍率）
    float intensity; // ノイズの明るさ（0.0～1.0）
    float useImage; // 元画像に乗算するか（1.0=使う、0.0=ノイズのみ）
}

// ===== 疑似乱数生成関数 =====
// 入力座標から 0.0～1.0 の乱数を生成する
float rand2dTo1d(float2 coord)
{
    return frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453);
}

// ===== 入力画像用 =====
Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

// ===== 出力構造体 =====
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ===== ピクセルシェーダー本体 =====
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV座標 * scale * time によって変化するノイズを作成
    float2 seed = input.texcoord * scale * time;
    float noise = rand2dTo1d(seed);

    if (useImage >= 1.0f)
    {
        // 元画像を取得し、ノイズと乗算して出力（ちらつき演出）
        float4 srcColor = gTexture.Sample(gSamplerLinear, input.texcoord);
        output.color = srcColor * noise * intensity;
    }
    else
    {
        // ノイズ単体で出力（完全なノイズ画面）
        float v = noise * intensity;
        output.color = float4(v, v, v, 1.0f);
    }

    return output;
}
