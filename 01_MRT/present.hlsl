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

float rand(float2 n) { 
    return frac(sin(dot(n, float2(12.9898, 4.1414))) * 43758.5453);
}

float3 GetNormal(float2 uv) {
	return NormalTex.Sample(ColorSmp, uv).xyz;// * 0.5 - 0.5;
}

float3 GetPos(float2 uv) {
	return PositionTex.Sample(ColorSmp, uv).xyz;
}

float GetDepth(float2 uv) {
	return DepthTex.Sample(ColorSmp, uv).x;
}

float GetAO(float2 uv) {

	const float total_strength = 2.0;
	const float base = 0.0;
	
	const float area = 0.0001;
	const float falloff = 0.00005;
	float radius = 0.0002;
	
	const int samples = 16;
	float3 sample_sphere[samples];
	sample_sphere[1] = float3( 0.5381, 0.1856,-0.4319); sample_sphere[9] = float3( 0.1379, 0.2486, 0.4430);
	sample_sphere[2] = float3( 0.3371, 0.5679,-0.0057); sample_sphere[10] = float3(-0.6999,-0.0451,-0.0019);
	sample_sphere[3] = float3( 0.0689,-0.1598,-0.8547); sample_sphere[11] = float3( 0.0560, 0.0069,-0.1843);
	sample_sphere[4] = float3(-0.0146, 0.1402, 0.0762); sample_sphere[12] = float3( 0.0100,-0.1924,-0.0344);
	sample_sphere[5] = float3(-0.3577,-0.5301,-0.4358); sample_sphere[13] = float3(-0.3169, 0.1063, 0.0158);
	sample_sphere[6] = float3( 0.0103,-0.5869, 0.0046); sample_sphere[14] = float3(-0.0897,-0.4940, 0.3287);
	sample_sphere[7] = float3( 0.7119,-0.0154,-0.0918); sample_sphere[15] = float3(-0.0533, 0.0596,-0.5411);
	sample_sphere[8] = float3( 0.0352,-0.0631, 0.5460); sample_sphere[0] = float3(-0.4776, 0.2847,-0.0271);
	
	float depth = GetDepth(uv);
	float3  world_pos = GetPos(uv);

   
	float3 position = float3(uv, depth);
	float3 normal = GetNormal(uv);
	float occlusion = 0.0;
	for(int r = 0; r < 8; r++) {
		float radius_depth = radius / depth;
		for(int i=0; i < samples; i++) {
			float3 random = normalize(
				float3(
					rand(Proj[2].xy * world_pos.yz * float2(i * r * 2.0, -i * r * 2.0)) * 2.0 - 1.0, 
					rand(Proj[2].yx * world_pos.zx * float2(-i * r * 4.0, i * r * 6.0)) * 2.0 - 1.0, 
					rand(Proj[2].xy * world_pos.xy * float2(i * r * 3.0, -i * r * 5.0)) * 2.0 - 1.0
				)
			);
			random = float3(0, 0, 0);
			float2 rand_uv = rand(Proj[2].xy * world_pos.yz * float2(i * r * 2.0, -i * r * 2.0));
			
			random = RandomTex.Sample(ColorSmp, rand_uv).xyz;
			float3 ray = radius_depth * reflect(sample_sphere[i], random);
			float3 hemi_ray = position + sign(dot(ray,normal)) * ray;
			float2 auv = clamp(hemi_ray.xy, float2(0, 0), float2(1, 1));
			float occ_depth = GetDepth(auv);
			float difference = depth - occ_depth;
			occlusion += step(falloff, difference) * (1.0 - smoothstep(falloff, area, difference));
		}
		radius *= 1.3;
	}
	occlusion /= 16.0;
	float ao = 1.0 - total_strength * occlusion * (1.0 / samples);
	return max(ao * ao, 0.0);
}


[RootSignature(RSDEF)]
PSOutput PSMain(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
	float3 texinfo0;
	float3 texinfo1;
	float3 texinfo2;
	float3 texinfo3;
	ColorTexture0.GetDimensions(0, texinfo0.x, texinfo0.y, texinfo0.z);
	NormalTex.GetDimensions(0, texinfo1.x, texinfo1.y, texinfo1.z);
	PositionTex.GetDimensions(0, texinfo2.x, texinfo2.y, texinfo2.z);
	DepthTex.GetDimensions(0, texinfo3.x, texinfo3.y, texinfo3.z);
	float2 uv       = input.Position.xy / texinfo0.xy;
	float4 diffuse  = ColorTexture0.Sample(ColorSmp, uv);
	float4 normal   = NormalTex.Sample(ColorSmp, uv);
	float4 position = PositionTex.Sample(ColorSmp, uv);
	float4 depth    = DepthTex.Sample(ColorSmp, uv);
	output.Color0   = diffuse;
	
	//
	float D = max(0, dot(normal.xyz, normalize(float3(1,2,-3))));
	float3 AO = GetAO(uv);
	output.Color0.w = 1.0;
	if(input.Position.x > Screen.x / 2) {
		output.Color0 = ColorTex.Sample(ColorSmp, input.Position.xy / texinfo0.xy, 1.0);
		output.Color0 = NormalTex.Sample(ColorSmp, input.Position.xy / texinfo1.xy, 1.0);
		if(input.Position.y < Screen.y / 2) {
			output.Color0 = PositionTex.Sample(ColorSmp, input.Position.xy / texinfo2.xy, 1.0);
		}
	} else {
		output.Color0 = DepthTex.Sample(ColorSmp, input.Position.xy / texinfo3.xy, 1.0);
		if(input.Position.y > Screen.y / 2) {
			float3 AO = GetAO(uv);
			output.Color0.xyz = AO;//float4(AO, AO, AO, 1.0);
			output.Color0.w = 1.0;
		}
	}
	output.Color0.xyz *= AO;//float4(AO, AO, AO, 1.0);

	//output.Color0.xyz *= RandomTex.Sample(ColorSmp, uv).xyz;
	output.Color0.a = 1.0;
	
    return output;
}


