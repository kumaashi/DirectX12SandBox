#include "common.hlsl"


float2 rotate(float2 p, float a) {
	return float2(
		p.x * cos(a) - p.y * sin(a),
		p.x * sin(a) + p.y * cos(a));
}

[RootSignature(RSDEF)]
VSOutput VSMain(const VSInput input)
{
	VSOutput output = (VSOutput)0;
	InstancingData instancingdata  = instdata[input.Id];
	output.Position      = float4(input.Position, 1.0f);
	output.Normal        = input.Normal;
	output.TexCoord      = input.TexCoord;
	output.Color         = instancingdata.color;
	output.WorldPosition = input.Id;
	output.TransPosition = output.Position;
	output.Id            = input.Id;
	return output;
}

[maxvertexcount(3)]
void GSMain(triangle VSOutput inputvertex[3], inout TriangleStream<VSOutput> gsstream ) {
	VSOutput output = (VSOutput)0;
	float3 v0 = inputvertex[0].Position;
	float3 v1 = inputvertex[1].Position;
	float3 v2 = inputvertex[2].Position;
	float3 N = normalize(cross(v2 - v0, v2 - v1));
	for(int i = 0; i < 3; i++) {
		VSOutput input = inputvertex[i];
		InstancingData instancingdata  = instdata[input.Id];
		float4x4 World       = instancingdata.world;
		float4x4 View        = matrixdata.View;
		float4x4 Proj        = matrixdata.Proj;
		float4   Position    = input.Position;
		//Position.xy = rotate(Position.xy, matrixdata.Time.x + input.Id);
		//Position.yz = rotate(Position.yz, matrixdata.Time.x * 2.0 + input.Id);
		/*
		float4 wP            = mul(Position, World);
		float4 wvP           = mul(wP, View);
		float4 wvpP          = mul(wvP, Proj);
		*/
		float4 wP            = mul(World, Position);
		float4 wvP           = mul(View , wP);
		float4 wvpP          = mul(Proj , wvP);
		/*
		float4 wN            = mul(float4(N, 0.0), World);
		float4 wvN           = mul(wN, View);
		float4 wvpN          = mul(wvN, Proj);
		*/
		float4 wN            = mul(World, float4(N, 0.0));
		float4 wvN           = mul(View , wN);
		float4 wvpN          = mul(Proj , wvN);
		output.Position      = wvpP;
		output.Normal        = normalize(wvpN.xyz);

		output.TexCoord      = input.TexCoord;
		output.Color         = instancingdata.color;
		output.WorldPosition = Position;
		output.TransPosition = output.Position;
		output.ViewPosition  = mul(matrixdata.View, Position);
		gsstream.Append(output);
  	}
  	gsstream.RestartStrip();
}

[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
	float3 N = input.Normal;
	float  Z = input.TransPosition.z / input.TransPosition.w;
	
	float3 V = normalize(-input.ViewPosition.xyz);
	float3 L = normalize(float3(-1, 1,-1));
	float3 H = normalize(L + V);
	float3 D = max(0.0, dot(N, L));
	float3 S = pow(max(0.0, dot(H, L)), 16.0);
	float3 C = input.Color.xyz;
	
	output.Color0 = float4(D * C + S * C, 1.0);
	output.Color1 = float4(N, 1.0);
	output.Color2 = float4(input.WorldPosition.xyz, 1.0);
	output.Color3 = float4(input.ViewPosition.xyz, Z);
    return output;
}


