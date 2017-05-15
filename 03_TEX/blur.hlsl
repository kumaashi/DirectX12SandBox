#include "common.hlsl"

[RootSignature(RSDEF)]
VSOutput VSMain(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4(input.Position, 1.0f);
    output.Position = localPos;
    output.Normal   = input.Normal;
    output.TexCoord = input.TexCoord;
    output.Color    = input.Color;

    return output;
}

//http://dev.theomader.com/gaussian-kernel-calculator/
float4 blur_m(float2 uv, float2 resolution, float2 direction) {
	float weight[11];
	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	weight[0] = 0.132429;
	weight[1] = 0.125337;
	weight[2] = 0.106259;
	weight[3] = 0.080693;
	weight[4] = 0.054891;
	weight[5] = 0.033446;
	weight[6] = 0.018255;
	weight[7] = 0.008925;
	weight[8] = 0.003908;
	weight[9] = 0.001533;
	weight[10] = 0.000539;
	for(int i = 0 ; i < 11; i++) {
		float2 duv = (float(i) * direction) / resolution * 4.0;
		color += PrevTex.Sample(ColorSmp, uv + duv) * weight[i];
		color += PrevTex.Sample(ColorSmp, uv - duv) * weight[i];
	}

	return color;
}



[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
    float3 texinfo0;
	PrevTex.GetDimensions(0, texinfo0.x, texinfo0.y, texinfo0.z);
	float2 uv       = input.Position.xy / texinfo0.xy;
	float2 dir      = float2(0.0, 1.0);
	if(Effect.x > 0.5) {
		dir = dir.yx;
	}
	output.Color0   = blur_m(uv, texinfo0.xy, dir);
    return output;
}


