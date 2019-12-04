//RDT0
Texture2D<float4> ColorTexture0 : register(t0);
SamplerState SamplerPoint : register(s0);
SamplerState SamplerLinear : register(s1);

struct vs_in
{
	float3  pos : pos;
	float2  uv  : uv;
	float3  nor : nor;
};

struct vs_out
{
	float4  pos : SV_POSITION;
	float4  uv  : TEXCOORD0;
};

struct ps_out
{
	float4  Color0  : SV_TARGET0;
	float4  Color1  : SV_TARGET1;
	float4  Color2  : SV_TARGET2;
	float4  Color3  : SV_TARGET3;
};

vs_out VSMain(vs_in ins, uint vid : SV_VertexID)
{
	vs_out output = (vs_out)0;
	uint id = vid % 4;
	if(id == 0) output.uv = float4(-1, -1, 0, 1);
	if(id == 1) output.uv = float4(-1,  1, 0, 1);
	if(id == 2) output.uv = float4( 1, -1, 0, 1);
	if(id == 3) output.uv = float4( 1,  1, 0, 1);
	output.pos = float4(ins.pos, 1.0);
	return output;
}


float2 rot(float2 p, float a) {
	float c = cos(a);
	float s = sin(a);
	return float2(
		p.x * c - p.y * s,
		p.x * s + p.y * c);
}

ps_out PSMain(const vs_out input)
{
	ps_out output = (ps_out)0;
	output.Color0 = float4(1, 1, 1, 1);
	float2 auv = rot(input.uv.xy, 0.3333);
	float2 uv = auv * 0.5 + 0.5;
	float2 duv = abs(input.uv.xy);
	float m = 2.0 - dot(duv, duv);
	output.Color0 = ColorTexture0.SampleLevel(SamplerLinear, uv, m * m * m);
	return output;
}



