Texture2D<float4> tex0 : register(t0);
SamplerState PointSampler   : register(s0);
SamplerState LinearSampler  : register(s1);

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : TEXCOORD0;
};

PSInput VSMain(float4 position : POSITION)
{
	PSInput result = (PSInput)0;
	float2 uv = position.xy * 0.5 + 0.5;
	result.position = position;
	result.color = uv.xyxy;
	//result.position.xy *= 0.5;
	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return tex0.SampleLevel(LinearSampler, input.color.xy, 0).zyxw;
}
