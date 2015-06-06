Texture2D fontTexture_ : register( t0 );
SamplerState sampler_ : register( s0 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
}

cbuffer FrameCB : register( b1 )
{
	matrix vp;
}

cbuffer ModelCB : register( b2 )
{
	float4 translate;
}

struct VS_Input
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float4 normal : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

PS_Input VSMain( VS_Input input )
{
	PS_Input output;
	output.pos.xyz = input.pos;
	output.pos.w = 1.0f;

//	output.pos = mul( output.pos, world );
//	output.pos += translate;
	output.pos = mul( output.pos, vp );

	output.normal.xyz = input.normal;
	output.normal.w = 1.0f;

	output.texcoord = input.texcoord*0.5;

	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float4 negLightDir = normalize( float4( 0.5f, 0.8f, 0.25f, 0.0f ) );

	float nDotL = dot( input.normal, negLightDir );
	float4 texSample =  fontTexture_.Sample( sampler_, input.texcoord );

	float4 sun = float4( 1.0f, 1.0f, 0.9f, 1.0f );
	float4 sky = float4( 0.5f, 0.75f, 0.9f, 1.0f );

	float4 lambert = texSample * nDotL * sun;
	float4 ambient = texSample * sky;

	return saturate( 0.8*ambient + 0.4*lambert );
	//return saturate( texSample*nDotL + texSample*float4( 0.1f, 0.1f, 0.23f, 1.0f )*2 );
}