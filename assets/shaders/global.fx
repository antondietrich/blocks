Texture2D textureA_ : register( t0 );
Texture2D shadowmap_ : register( t1 );
Texture2D lightColor : register( t2 );

SamplerState samplerFiltered : register( s0 );
SamplerState samplerPoint : register( s1 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[16];
}

cbuffer FrameCB : register( b1 )
{
	matrix vp;
	matrix lightVP;
	float4 sunDir;
	float4 sunColor;
	float4 ambientColor;
	float dayTimeNorm;
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
	float4 lightViewPos : TEXCOORD0;
//float4 pos : TEXCOORD0;
//float4 lightViewPos : SV_POSITION;
	float4 normal : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
	float occlusion : TEXCOORD3;
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
	output.lightViewPos = output.pos;
	output.pos = mul( output.pos, vp );

	output.lightViewPos = mul( output.lightViewPos, lightVP );

	output.normal = normals[ normalIndex ];

	output.texcoord = texcoords[ texcoordIndex ];// * 0.5;

	output.occlusion = occluded / 3.0; //input.ao;// / 3.0;

	// output.texID = input.texID;

	return output;
}

bool IsPointInsideProjectedVolume( float3 P );

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float2 tc;// = float2( input.lightViewPos.x, input.lightViewPos.y );
	tc.x = input.lightViewPos.x / input.lightViewPos.w / 2.0f + 0.5f;
	tc.y = -input.lightViewPos.y / input.lightViewPos.w / 2.0f + 0.5f;

	// return float4( input.lightViewPos.x, input.lightViewPos.y, 0.0, 1.0 );
	// return sm;

	// return float4( 1.0f, 1.0f, 1.0f, 1.0f );

	float4 negLightDir = normalize( sunDir );
	float ao = ( 1.0 - input.occlusion ) * 0.7 + 0.3;

	float4 sunColorTex = lightColor.Sample( samplerFiltered, float2( dayTimeNorm, 0.25 ) );
	float4 ambientColorTex = lightColor.Sample( samplerFiltered, float2( dayTimeNorm, 0.75 ) );

	float nDotL = saturate( dot( input.normal, negLightDir ) );
	
	if( IsPointInsideProjectedVolume( input.lightViewPos ) )
	{
		float maxBias = 0.001;
		float slope = dot( input.normal, negLightDir );
		if( slope < 0 )
		{
			slope = 0;
		}
		else
		{
			slope = 1 - slope;
		}

		float bias = maxBias * slope + 0.00001;

		float sm = shadowmap_.Sample( samplerPoint, tc );

		if( input.lightViewPos.z / input.lightViewPos.w - bias > sm )
		{
//			return float4( 0.0, 0.0, 1.0, 1.0 );
			nDotL = 0;
		}
	}

	float lighting = nDotL * sunColorTex + ao * ambientColorTex;
	float4 color = textureA_.Sample( samplerFiltered, input.texcoord );

	return color * nDotL * sunColorTex * 0.5 + color * ao * ambientColorTex * 0.7;
}

bool IsPointInsideProjectedVolume( float3 P )
{
	return saturate( abs( P.x ) ) == abs( P.x ) && 
	       saturate( abs( P.y ) ) == abs( P.y ) &&
	       saturate( P.z ) 		  == P.z;
}