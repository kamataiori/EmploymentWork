#include "Fullscreen.hlsli"

// 画面全体を「横」＋「左上→右下の斜め」の2本の斬撃ラインで最大4片に切断し、
// 最後に割れたガラスのように各片を重力＋回転で落下させるポストエフェクト。
//   hProgress : 横斬りの分離進捗   0..1（>0で横ライン有効）
//   dProgress : 斜め斬りの分離進捗 0..1（>0で斜めライン有効）
//   fall      : ガラス落下進捗     0..1
//   edge      : 断面の発光ライン幅
//   hSep/dSep : 横／斜めの最大分離量
//   slide     : 横方向スライド最大量
cbuffer SlashCutSettings : register(b0)
{
    float hProgress;
    float dProgress;
    float fall;
    float edge;
    float hSep;
    float dSep;
    float slide;
    float pad;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

static const float kSqrt2Inv = 0.70710678f;

// 片ごとの落下方向（x:横ドリフト / y:重力方向の相対速度）と回転量
static const float2 kFallDir[4] =
{
    float2(0.10f, 1.00f), // id0
    float2(-0.10f, 1.05f), // id1
    float2(0.18f, 1.20f), // id2
    float2(-0.18f, 1.15f), // id3
};
static const float kSpin[4] = { -0.35f, 0.30f, 0.45f, -0.50f };

// 点 p が属する片ID（非アクティブなラインでは分割しない）
int PieceId(float2 p)
{
    int a = (hProgress > 0.0f && p.y > 0.5f) ? 1 : 0; // 横ライン(y=0.5)
    int b = (dProgress > 0.0f && p.y > p.x) ? 1 : 0; // 斜めライン(y=x)
    return a * 2 + b;
}

// 片ごとの分離オフセット（落下前に開く隙間）
float2 SepOffset(int id)
{
    int a = id / 2; // 0=上(y<0.5) / 1=下
    int b = id % 2; // 0=(y<x) / 1=(y>x)

    float2 o = float2(0.0f, 0.0f);

    // 横：上は上＆左へ、下は下＆右へ
    float hs = hSep * hProgress;
    float ls = slide * hProgress;
    float hDir = (a == 0) ? -1.0f : 1.0f;
    o += float2(hDir * ls, hDir * hs);

    // 斜め：法線(0.707,-0.707)に沿って両側へ
    float ds = dSep * dProgress;
    float dDir = (b == 0) ? 1.0f : -1.0f;
    o += dDir * ds * float2(kSqrt2Inv, -kSqrt2Inv);

    return o;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    const float2 pivot = float2(0.5f, 0.5f);

    float3 col = float3(0.0f, 0.0f, 0.0f);

    // 4片それぞれについて「この画面画素を覆っているか」を逆変換で判定
    [unroll]
    for (int id = 0; id < 4; ++id)
    {
        // 存在しない片はスキップ（該当ライン非アクティブ）
        if (dProgress <= 0.0f && (id % 2) == 1)
            continue;
        if (hProgress <= 0.0f && (id / 2) == 1)
            continue;

        float ang = kSpin[id] * fall;
        float2 trans = SepOffset(id)
            + float2(kFallDir[id].x * fall, kFallDir[id].y * fall * fall); // 重力は f^2

        // 逆変換：screen = pivot + Rot(ang)*(src-pivot) + trans
        float2 q = uv - trans - pivot;
        float cs = cos(-ang);
        float sn = sin(-ang);
        float2 src = pivot + float2(q.x * cs - q.y * sn, q.x * sn + q.y * cs);

        // 元画像の外／別の片ならこの片は覆っていない
        if (src.x < 0.0f || src.x > 1.0f || src.y < 0.0f || src.y > 1.0f)
            continue;
        if (PieceId(src) != id)
            continue;

        col = gTexture.Sample(gSampler, src).rgb;
        break;
    }

    // 断面の発光ラインは無し（切れ目に線を出さない）

    output.color = float4(col, 1.0f);
    return output;
}
