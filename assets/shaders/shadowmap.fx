cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[16];
}

cbuffer LightCB : register( b1 )
{
	matrix vp;
}

cbuffer ModelCB : register( b2 )
{
	float4 translate;
}

struct VS_Input
{
	uint pos : POSITION;
	uint texID : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
};

PS_Input VSMain( VS_Input input )
{
	// unpack position, normal texcoord
	float3 unpackedPos;
	unpackedPos.x = ( input.pos & 0x000000ff );
	unpackedPos.y = ( input.pos & 0x0000ff00 ) >> 8;
	unpackedPos.z = ( input.pos & 0x00ff0000 ) >> 16;

	PS_Input output;

	output.pos.xyz = unpackedPos;
	output.pos.w = 1.0f;

	output.pos += translate;
	output.pos = mul( output.pos, vp );

	return output;
}

float PSMain( PS_Input input ) : SV_TARGET
{
	return input.pos.z;
}