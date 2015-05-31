Texture2D fontTexture_ : register( t0 );
SamplerState sampler_ : register( s0 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
}

cbuffer OverlayCB : register( b1 )
{
	float4 color;
}

struct VS_Input
{
	float2 pos : POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

PS_Input VSMain( VS_Input input )
{
	PS_Input output;
	output.pos.xy = input.pos;
	output.pos.z = 0.5f;
	output.pos.w = 1.0f;

	output.pos = mul( output.pos, screenToNDC );

	output.texcoord = input.texcoord;
	
	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float4 outColor;
	float4 texSample =  fontTexture_.Sample( sampler_, input.texcoord );
	outColor.rgb = color.rgb;
	outColor.a = texSample.a;
	return outColor;
}