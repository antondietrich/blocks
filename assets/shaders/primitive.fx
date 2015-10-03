cbuffer FrameCB : register( b1 )
{
	matrix vp;
}

struct VS_Input
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

PS_Input VSMain( VS_Input input )
{
	PS_Input output;
	output.pos.xyz = input.pos;
	output.pos.w = 1.0f;
	output.pos = mul( output.pos, vp );
	output.color = input.color;
	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	return input.color;
}