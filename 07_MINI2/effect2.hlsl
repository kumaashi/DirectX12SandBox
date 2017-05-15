#include "common.hlsl"

[RootSignature(RSDEF)]
VSOutput VSMain(const VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Position = float4(input.Position * float3(1,1,0), 1.0f);
	output.Normal   = input.Normal;
	output.Color	= float4(1,1,1,1);
	output.TexCoord = output.Position.xy;
	return output;
}

[maxvertexcount(3)]
void GSMain(triangle VSOutput inputvertex[3], inout TriangleStream<VSOutput> gsstream ) {
	VSOutput output = (VSOutput)0;
	for(int i = 0; i < 3; i++) {
		VSOutput input = inputvertex[i];
		output.Position	  = input.Position;
		output.Normal		= input.Normal;
		output.TexCoord	  = input.TexCoord;
		gsstream.Append(output);
	}
	gsstream.RestartStrip();
}

[RootSignature(RSDEF)]
#define BLUR_POWER 12.0



float4 GetColor(float2 uv) {
	return ColorTexture0.Sample(ColorSmp, uv);
}

PSOutput PSMain(const VSOutput input)
{
	PSOutput output = (PSOutput)0;
	
	float3 texinfo0;
	ColorTexture0.GetDimensions(0, texinfo0.x, texinfo0.y, texinfo0.z);
	float2 uv = input.Position.xy / texinfo0.xy;
	
	float2 duv = (uv * 2.0) - 1.0;
	float2 center = texinfo0.xy * 0.5;
	float vig;
	float4 col1 = float4(0, 0, 0, 0);
	float4 col2 = float4(0, 0, 0, 0);
	float4 col3 = float4(0, 0, 0, 0);
	float4 col4 = float4(0, 0, 0, 0);
	vig = distance(center, input.Position.xy) / texinfo0.x;
	float2 pixel_size = 1.0 / (texinfo0.xy + 0.5) * (vig * BLUR_POWER);
	col1 += GetColor( uv );
	col2 += GetColor( uv + float2(0.0, pixel_size.y));
	col3 += GetColor( uv + float2(pixel_size.x, 0.0));
	col4 += GetColor( uv + pixel_size);
	float2 slide = float2(0.005, 0.0) * vig + float2(0.001, 0.0);
	float4 col5 = saturate(GetColor(uv - slide));
	float4 col6 = saturate(GetColor(uv + slide));
	col1.r += col5.r;
	col1.b += col6.b;

	col2.r += col5.r;
	col2.b += col6.b;

	col3.r += col5.r;
	col3.b += col6.b;

	col4.r += col5.r;
	col4.b += col6.b;
	col1 = saturate(col1);
	col2 = saturate(col2);
	col3 = saturate(col3);
	col4 = saturate(col4);

	output.Color0 = (col1 + col2 + col3 + col4) * 0.25;

	output.Color0 *= (1.0 - dot(duv * 0.5, duv));
	return output;
}


