Texture2D textureA_ : register( t0 );
SamplerState sampler_ : register( s0 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[16];
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
	uint texID : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float4 normal : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
	float occlusion : TEXCOORD2;
	// float texID : TEXCOORD3;
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

//	output.pos = mul( output.pos, world );
	output.pos += translate;
	output.pos = mul( output.pos, vp );

	output.normal = normals[ normalIndex ];

	output.texcoord = texcoords[ texcoordIndex ];// * 0.5;

	output.occlusion = occluded / 3.0; //input.ao;// / 3.0;

	// output.texID = input.texID;

	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float ao = ( 1.0 - input.occlusion ) * 0.7 + 0.3;

	//return 0.2 * ao * input.texID;

	float4 negLightDir = normalize( float4( 0.5f, 0.8f, 0.25f, 0.0f ) );
	float nDotL = dot( input.normal, negLightDir ) * 0.3 + 0.7;

	float4 texSample =  textureA_.Sample( sampler_, input.texcoord );

	//return ao;
	return ao * texSample * nDotL;

	float4 sun = float4( 1.0f, 1.0f, 0.9f, 1.0f );

//	float ao = saturate( ( 1 - input.occlusion )*( 1 - input.occlusion ) + 0.7 );
//	float4 lambert = texSample * nDotL * sun;
//	float4 ambient = texSample + texSample*ao*0.5;// - ao;

//	return saturate( 0.8*ambient + 0.2*lambert);
	//return saturate( texSample*nDotL + texSample*float4( 0.1f, 0.1f, 0.23f, 1.0f )*2 );
}