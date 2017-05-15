#include "common.hlsl"

[RootSignature(RSDEF)]
VSOutput VSMain(uint id : SV_VertexID, const VSInput input)
{
	VSOutput output = (VSOutput)0;
	if(id == 0) output.Position = float4(-1, 1, 0, 1);
	if(id == 1) output.Position = float4( 1, 1, 0, 1);
	if(id == 2) output.Position = float4( 1,-1, 0, 1);

	if(id == 3) output.Position = float4(-1, 1, 0, 1);
	if(id == 4) output.Position = float4(-1,-1, 0, 1);
	if(id == 5) output.Position = float4( 1,-1, 0, 1);
	
	
	output.Color    = float4(1,1,1,1);
	output.TexCoord = output.Position.xy;
	return output;
}

/*
[maxvertexcount(3)]
void GSMain(triangle VSOutput inputvertex[3], inout TriangleStream<VSOutput> gsstream ) {
	VSOutput output = (VSOutput)0;
	for(int i = 0; i < 3; i++) {
		VSOutput input  = inputvertex[i];
		output.Position = input.Position;
		output.Normal   = input.Normal;
		output.TexCoord = input.TexCoord;
		gsstream.Append(output);
  	}
  	gsstream.RestartStrip();
}
*/

[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input) {
    PSOutput output = (PSOutput)0;
	float2 uv = (input.TexCoord) * 0.5 + 0.5;
	uv.y = 1.0 - uv.y;
	output.Color0 = float4(uv, 0,1) * saturate(ColorTexture0.SampleLevel(ColorSmp, uv, 0.0));
    return output;
}


