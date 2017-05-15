struct VSInput
{
    float3  Position : POSITION;
    float3  Normal   : NORMAL;
    float2  TexCoord : UV;
    float4  Color    : COLOR;
};

struct VSOutput
{
    float4  Position      : SV_POSITION;
    float3  Normal        : NORMAL;
    float2  TexCoord      : TEXCOORD0;
    float4  Color         : COLOR;
    float4  WorldPosition : TEXCOORD1;
    float4  TransPosition : TEXCOORD2;
};

struct PSOutput
{
	float4  Color0  : SV_TARGET0; //albedocolor
	float4  Color1  : SV_TARGET1; //worldnormal
	float4  Color2  : SV_TARGET2; //worldposition
	float4  Color3  : SV_TARGET3; //depth
};

cbuffer TransformBuffer : register(b0)
{
    float4x4 World  : packoffset(c0);
    float4x4 View   : packoffset(c4);
	float4x4 Proj   : packoffset(c8);
	float4x4 Temp   : packoffset(c12);
	float4   Screen : packoffset(c16);
	float4   Time   : packoffset(c17);
	float4   Pos    : packoffset(c18);
	float4   At     : packoffset(c19);
	float4   Up     : packoffset(c20);
};

//RootSignature
#define RSDEF "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),  DescriptorTable(CBV(b0)), DescriptorTable(SRV(t0)), DescriptorTable(SRV(t1)), DescriptorTable(SRV(t2)), DescriptorTable(SRV(t3)), DescriptorTable(SRV(t4)), StaticSampler(s0)"

Texture2D       ColorTexture0 : register(t0); //albedocolor
Texture2D       ColorTexture1 : register(t1); //worldnormal
Texture2D       ColorTexture2 : register(t2); //worldposition
Texture2D       ColorTexture3 : register(t3); //depth
Texture2D       ColorTexture4 : register(t4); //randomtex

#define ColorTex    ColorTexture0 
#define NormalTex   ColorTexture1 
#define PositionTex ColorTexture2 
#define DepthTex    ColorTexture3 
#define RandomTex   ColorTexture4 

//GL
#define texture2D(tex, uv) tex.Sample(ColorCmp, uv)
#define texture(tex, uv)   tex.Sample(ColorCmp, uv)

SamplerState    ColorSmp      : register(s0 );

