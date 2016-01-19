#include "include/lambert.fx"

Texture2D textureAlbedo : register( t0 );
SamplerState samplerFiltered : register( s0 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[4];
	float viewDistance;
	float padding[3];
}

cbuffer FrameCB : register( b1 )
{
	matrix vp;
	matrix lightVP[4];
	float4 sunDir;
	float4 sunColor;
	float4 ambientColor;
	float4 smTexelSize;
	float dayTimeNorm;
}

cbuffer ModelCB : register( b2 )
{
	matrix world;
	matrix rotation;
	float3 translation;
	float3 rotationEuler;
	float scale;
	float mcb_padding;
}

struct VS_Input
{
	float3 position : POSITION;
	float3 normal : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

struct PS_Input
{
	float4 position : SV_POSITION;
	float4 normal : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

PS_Input VSMain( VS_Input input )
{
	PS_Input output;
	output.position.xyz = input.position;
	output.position.w = 1.0f;
	output.position = mul( output.position, world );
	output.position = mul( output.position, vp );

	output.normal = mul( input.normal, rotation );

	output.texcoord = input.texcoord;
	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float4 color = textureAlbedo.Sample( samplerFiltered, input.texcoord );
	float4 negLightDir = normalize( sunDir );
	float nDotL = Lambert( input.normal, negLightDir );

	float4 finalColor = color * nDotL * 0.8 +
						color * 0.2;

	return finalColor;
}
