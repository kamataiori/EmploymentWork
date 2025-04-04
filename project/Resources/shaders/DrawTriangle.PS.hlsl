struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.color.rgb, input.color.a); // アルファ値を適用
}
