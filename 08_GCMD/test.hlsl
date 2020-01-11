Texture2D<float4> tex0 : register(t0);
Texture2D<float4> tex1 : register(t1);
SamplerState PointSampler   : register(s0);
SamplerState LinearSampler  : register(s1);

cbuffer constdata : register(b0)
{
	float4 color;
	float4 misc;
};

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
	result.position.xy *= 0.95;
	result.color = uv.xyxy;
	return result;
}

float2 rot(float2 p, float a) {
	float c = cos(a);
	float s = sin(a);
	return float2(
		c * p.x - s * p.y,
		s * p.x + c * p.y);
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float tm = misc.x * 0.5;
	float2 jitter = float2(cos(tm), sin(tm));
	input.color.xy = rot(input.color.xy, tm);
	return 
	cos(tex0.SampleLevel(LinearSampler, 2.0 * cos(input.color.xy) + jitter, 0)) -
		
	(tex1.SampleLevel(LinearSampler, exp(input.color.xy) + jitter, 0) * float4(1,2,3,0.4) * 0.99);
}
