Texture2D fontTexture_ : register( t0 );
SamplerState sampler_ : register( s0 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[4];
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
	uint pos : POSITION;
	uint normal : TEXCOORD0;
	uint texcoord : TEXCOORD1;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float4 normal : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
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
	int texcoordIndex = ( temp & 0x1c ) >> 2; 	// 00011100
	int occluded = ( temp & 0x02 ) >> 1;		// 00000010

	PS_Input output;

	output.pos.xyz = unpackedPos;
	output.pos.w = 1.0f;

//	output.pos = mul( output.pos, world );
	output.pos += translate;
	output.pos = mul( output.pos, vp );

	output.normal = normals[ normalIndex ];

	output.texcoord = texcoords[ texcoordIndex ];

	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float4 negLightDir = normalize( float4( 0.5f, 0.8f, 0.25f, 0.0f ) );

	float nDotL = dot( input.normal, negLightDir );
	float4 texSample =  fontTexture_.Sample( sampler_, input.texcoord );

	float4 sun = float4( 1.0f, 1.0f, 0.9f, 1.0f );

	float4 lambert = texSample * nDotL * sun;
	float4 ambient = texSample;

	return saturate( 0.8*ambient + 0.2*lambert );
	//return saturate( texSample*nDotL + texSample*float4( 0.1f, 0.1f, 0.23f, 1.0f )*2 );
}