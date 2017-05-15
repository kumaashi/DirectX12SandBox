#include "common.hlsl"

[RootSignature(RSDEF)]
VSOutput VSMain(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View,  worldPos);
    float4 projPos  = mul(Proj,  viewPos);

    output.Position = localPos;
    output.Normal   = input.Normal;
    output.TexCoord = input.TexCoord;
    output.Color    = input.Color;

    return output;
}


[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
	float3 texinfo0;
	ColorTexture0.GetDimensions(0, texinfo0.x, texinfo0.y, texinfo0.z);
	float2 uv       = input.Position.xy / texinfo0.xy;
	output.Color0.xyz = ColorTex.Sample(ColorSmp, uv).xyz;
	output.Color0.a = 1.0;
	
    return output;
}


