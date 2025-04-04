cbuffer TransformationMatrix : register(b0)
{
    float4x4 WVP;
    float4x4 World;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    // 位置を WVP 行列で変換
    output.position = mul(float4(input.position, 1.0), WVP);
    output.color = input.color; // 色をそのまま渡す
    return output;
}
