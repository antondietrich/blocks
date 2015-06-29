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

	float3 unpackedNormal;
	unpackedNormal.x = ( input.normal & 0x000000ff );
	unpackedNormal.y = ( input.normal & 0x0000ff00 ) >> 8;
	unpackedNormal.z = ( input.normal & 0x00ff0000 ) >> 16;

//	unpackedNormal.x = ( input.normal & 0xff000000 ) >> 24;
//	unpackedNormal.y = ( input.normal & 0x0000ff00 >> 8);
//	unpackedNormal.z = ( input.normal & 0x00ff0000 >> 16);

	float2 unpackedTexcoord;
	unpackedTexcoord.x = ( input.texcoord & 0x000000ff );
	unpackedTexcoord.y = ( input.texcoord & 0x0000ff00 ) >> 8;

	PS_Input output;

	output.pos.xyz = unpackedPos;
	output.pos.w = 1.0f;

//	output.pos = mul( output.pos, world );
	output.pos += translate;
	output.pos = mul( output.pos, vp );

	// NOTE: normals offset by +1 to avoid issues caused by 2's compliment notation
	//  (can't just shift right to get a negative number)
	output.normal.xyz = unpackedNormal - 1;
	output.normal.w = 1.0f;
//	output.normal = normalize( output.normal );

	output.texcoord = unpackedTexcoord;

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