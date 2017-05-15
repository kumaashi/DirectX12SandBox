#include "common.hlsl"

[RootSignature(RSDEF)]
VSOutput VSMain(const VSInput input)
{
	VSOutput output = (VSOutput)0;
	float4 apos          = float4(input.Position, 1.0f);
	output.Position      = mul(Proj, mul(View, (mul(World, apos))));;
	output.Normal        = input.Normal;
	output.TexCoord      = input.TexCoord;
	output.Color         = input.Color;
	output.WorldPosition = apos;
	output.TransPosition = output.Position;
	return output;
}

[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
	float3 N = input.Normal;
	float Z  = input.TransPosition.z / input.TransPosition.w;
	output.Color0 = input.Color;
	output.Color1 = float4(N, 1.0);
	output.Color2 = float4(input.WorldPosition.xyz, 1.0);
	output.Color3 = float4(Z, Z, Z, 1.0);
    return output;
}


