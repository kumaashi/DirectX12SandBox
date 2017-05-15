//RootSignature
#define RSDEF \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
	"DescriptorTable(CBV(b0))," \
	"DescriptorTable(CBV(b1))," \
	"DescriptorTable(SRV(t0))," \
	"DescriptorTable(SRV(t1))," \
	"DescriptorTable(SRV(t2))," \
	"DescriptorTable(SRV(t3))," \
	"DescriptorTable(SRV(t4))," \
	"StaticSampler(s0,"\
	"           filter = FILTER_MIN_MAG_MIP_LINEAR,"\
	"           addressU = TEXTURE_ADDRESS_CLAMP,"\
	"           addressV = TEXTURE_ADDRESS_CLAMP,"\
	"           addressW = TEXTURE_ADDRESS_CLAMP )"

struct VSInput
{
	float3  Position : POSITION;
	float3  Normal   : NORMAL;
	float2  TexCoord : UV;
	float4  Color    : COLOR;
	uint    Id       : SV_InstanceID;
};

struct VSOutput
{
	float4  Position      : SV_POSITION;
	float3  Normal        : NORMAL;
	float2  TexCoord      : TEXCOORD0;
	float4  Color         : COLOR;
	float4  WorldPosition : TEXCOORD1;
	float4  TransPosition : TEXCOORD2;
	float4  ViewPosition  : TEXCOORD3;
	uint    Id            : TEXCOORD4;
};


struct PSOutput
{
	float4  Color0  : SV_TARGET0;
	float4  Color1  : SV_TARGET1;
	float4  Color2  : SV_TARGET2;
	float4  Color3  : SV_TARGET3;
};

struct InstancingData {
	float4x4 world;
	float4   color;
};

struct MatrixData {
	float4x4 World  ;
	float4x4 View   ;
	float4x4 Proj   ;
	float4x4 padding;
	float4   Time;
};


cbuffer MatrixBuffer : register(b0) {
	MatrixData matrixdata;
};

cbuffer InstancingBuffer : register(b1) {
	InstancingData instdata[256];
};

Texture2D    ColorTexture0 : register(t0);
Texture2D    ColorTexture1 : register(t1);
Texture2D    ColorTexture2 : register(t2);
Texture2D    ColorTexture3 : register(t3);
Texture2D    ColorTexture4 : register(t4);
SamplerState ColorSmp      : register(s0);

