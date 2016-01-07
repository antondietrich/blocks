Texture2DArray blockTexture : register( t0 );
SamplerState samplerFiltered : register( s0 );

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
	uint data1 : POSITION;
	uint data2 : TEXCOORD0;
};

struct PS_Input
{
	float4 pos 		: SV_POSITION;
	float2 texcoord : TEXCOORD0;
	int texID 		: TEXCOORD1;
};

float ShowTexel( float4 fragmentPos );

PS_Input VSMain( VS_Input input )
{
	// unpack position, normal texcoord
	float3 unpackedPos;
	unpackedPos.x = ( input.data1 & 0x000000ff );
	unpackedPos.y = ( input.data1 & 0x0000ff00 ) >> 8;
	unpackedPos.z = ( input.data1 & 0x00ff0000 ) >> 16;

	PS_Input output;

	output.pos.xyz = unpackedPos;
	output.pos.w = 1.0f;

	output.pos += translate;
	output.pos = mul( output.pos, vp );

	int temp = ( input.data1 & 0xff000000 ) >> 24;
	int texcoordIndex = ( temp & 0x18 ) >> 3;
	output.texcoord = texcoords[ texcoordIndex ];

	temp = (input.data2 & 0x000000ff );
	output.texID = temp & 0x3f;

	return output;
}

float PSMain( PS_Input input ) : SV_DEPTH
{
	//return ShowTexel( input.pos );
	float3 texcoord;
	texcoord.xy = input.texcoord;
	texcoord.z = input.texID;
	float4 color = blockTexture.Sample( samplerFiltered, texcoord );

	if( color.a < 0.5 )
		discard;
	return input.pos.z;
}

//
// Produces a checkered map to inspect shadow map texel size
//
float ShowTexel( float4 fragmentPos )
{
	int2 pixelPos;
	pixelPos.x = (fragmentPos.x / fragmentPos.w) / 2  + 0.5;
	pixelPos.y = (fragmentPos.y / fragmentPos.w) / 2  + 0.5;

	if( (pixelPos.x % 2) != (pixelPos.y % 2) )
	{
		return 1.0;
	}
	return 0.0;
}
