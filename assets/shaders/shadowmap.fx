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

	int temp = ( input.pos & 0xff000000 ) >> 24;
	int normalIndex = ( temp & 0xe0 ) >> 5; 	// 11100000
	int texcoordIndex = ( temp & 0x1F ); 	// 00011111
	// int occluded = ( temp & 0x06 ) >> 1;		// 00000100

	int occluded = input.texID;

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