//Texture2DArray blockTexture : register( t0 );
Texture2D blockTexture[4] : register( t0 );
Texture2D shadowmap_[4] : register( t4 );
Texture2D lightColor : register( t8 );

SamplerState samplerFiltered : register( s0 );
SamplerState samplerPoint : register( s1 );

cbuffer GlobalCB : register( b0 )
{
	matrix screenToNDC;
	float4 normals[6];
	float4 texcoords[4];
	float viewDistance;
	float padding[3];
}

cbuffer FrameCB : register( b1 )
{
	matrix vp;
	matrix lightVP[4];
	float4 sunDir;
	float4 sunColor;
	float4 ambientColor;
	float4 smTexelSize;
	float dayTimeNorm;
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
	float4 pos 				: SV_POSITION;
	float4 lightViewPos[4]	: TEXCOORD0;
	float4 normal 			: TEXCOORD4;
	float2 texcoord 		: TEXCOORD5;
	float occlusion 		: TEXCOORD6;
	int texID 				: TEXCOORD7;
	float2 linearDepth 		: TEXCOORD8;
};

#define OFFSET_UV_ONLY 1

PS_Input VSMain( VS_Input input )
{
	// unpack position, normal texcoord
	float3 unpackedPos;
	unpackedPos.x = ( input.data1 & 0x000000ff );
	unpackedPos.y = ( input.data1 & 0x0000ff00 ) >> 8;
	unpackedPos.z = ( input.data1 & 0x00ff0000 ) >> 16;

	int temp = ( input.data1 & 0xff000000 ) >> 24;
	int normalIndex = ( temp & 0xe0 ) >> 5; 	// 11100000
	int texcoordIndex = ( temp & 0x18 ) >> 3; 	// 00011000
	// int occluded = ( temp & 0x06 ) >> 1;		// 00000100

	temp = (input.data2 & 0x000000ff );
	int occluded = (temp & 0xc0) >> 6;
	int texID = temp & 0x3f;

	PS_Input output;

	output.pos.xyz = unpackedPos;
	output.pos.w = 1.0f;

//	output.pos = mul( output.pos, world );
	output.pos += translate;

	output.lightViewPos[0] = output.pos;
	output.lightViewPos[1] = output.pos;
	output.lightViewPos[2] = output.pos;
	output.lightViewPos[3] = output.pos;

	float4 normal = normals[ normalIndex ];
	float4 negLightDir = normalize( sunDir );
	float normalOffset = 1.0;
	float normalOffsetScale = saturate( 1 - dot( normal, negLightDir ) );

#if OFFSET_UV_ONLY

	float4 offsetPosition[4];

	offsetPosition[0] = output.lightViewPos[0] + normal * normalOffset * normalOffsetScale * smTexelSize[0];
	offsetPosition[1] = output.lightViewPos[1] + normal * normalOffset * normalOffsetScale * smTexelSize[1];
	offsetPosition[2] = output.lightViewPos[2] + normal * normalOffset * normalOffsetScale * smTexelSize[2];
	offsetPosition[3] = output.lightViewPos[3] + normal * normalOffset * normalOffsetScale * smTexelSize[3];

	offsetPosition[0] = mul( offsetPosition[0], lightVP[0] );
	offsetPosition[1] = mul( offsetPosition[1], lightVP[1] );
	offsetPosition[2] = mul( offsetPosition[2], lightVP[2] );
	offsetPosition[3] = mul( offsetPosition[3], lightVP[3] );

	output.lightViewPos[0] = mul( output.lightViewPos[0], lightVP[0] );
	output.lightViewPos[1] = mul( output.lightViewPos[1], lightVP[1] );
	output.lightViewPos[2] = mul( output.lightViewPos[2], lightVP[2] );
	output.lightViewPos[3] = mul( output.lightViewPos[3], lightVP[3] );

	output.lightViewPos[0].xy = offsetPosition[0].xy;
	output.lightViewPos[1].xy = offsetPosition[1].xy;
	output.lightViewPos[2].xy = offsetPosition[2].xy;
	output.lightViewPos[3].xy = offsetPosition[3].xy;

#else

	output.lightViewPos[0] += normal * normalOffset * normalOffsetScale * smTexelSize[0];
	output.lightViewPos[1] += normal * normalOffset * normalOffsetScale * smTexelSize[1];
	output.lightViewPos[2] += normal * normalOffset * normalOffsetScale * smTexelSize[2];
	output.lightViewPos[3] += normal * normalOffset * normalOffsetScale * smTexelSize[3];

	output.lightViewPos[0] = mul( output.lightViewPos[0], lightVP[0] );
	output.lightViewPos[1] = mul( output.lightViewPos[1], lightVP[1] );
	output.lightViewPos[2] = mul( output.lightViewPos[2], lightVP[2] );
	output.lightViewPos[3] = mul( output.lightViewPos[3], lightVP[3] );

#endif

	output.pos = mul( output.pos, vp );
	output.normal = normals[ normalIndex ];

	output.texcoord = texcoords[ texcoordIndex ];// * 0.5;

	output.occlusion = occluded / 3.0; //input.ao;// / 3.0;

	output.texID = texID;

	output.linearDepth.x = output.pos.w / viewDistance;
	output.linearDepth.y = 1.0 - unpackedPos.y / 128;

	return output;
}

bool IsPointInsideProjectedVolume( float3 P );

float4 PSMain( PS_Input input ) : SV_TARGET
{
	float4 negLightDir = normalize( sunDir );
	float ao = ( 1.0 - input.occlusion ) * 0.7 + 0.3;

	float4 sunColorTex = lightColor.Sample( samplerFiltered, float2( dayTimeNorm, 0.25 ) );
	float4 ambientColorTex = lightColor.Sample( samplerFiltered, float2( dayTimeNorm, 0.75 ) );

	float nDotL = saturate( dot( input.normal, negLightDir ) );

	// pick cascade
	int sliceIndex = 0;
	for( sliceIndex = 0; sliceIndex < 4; sliceIndex++ )
	{
		if( IsPointInsideProjectedVolume( input.lightViewPos[ sliceIndex ] ) )
		{
			break;
		}
	}
	float4 cascadeColors[5];
	cascadeColors[0] = float4( 1.0, 0.0, 0.0, 1.0 ); // red
	cascadeColors[1] = float4( 0.0, 1.0, 0.0, 1.0 ); // green
	cascadeColors[2] = float4( 0.0, 0.0, 1.0, 1.0 ); // blue
	cascadeColors[3] = float4( 1.0, 1.0, 0.0, 1.0 ); // yellow
	cascadeColors[4] = float4( 1.0, 0.0, 1.0, 1.0 );

	float2 tc;
	tc.x =  input.lightViewPos[ sliceIndex ].x / input.lightViewPos[sliceIndex].w / 2.0f + 0.5f;
	tc.y = -input.lightViewPos[ sliceIndex ].y / input.lightViewPos[sliceIndex].w / 2.0f + 0.5f;

	float sm[4];
	sm[0] = shadowmap_[0].Sample( samplerPoint, tc );
	sm[1] = shadowmap_[1].Sample( samplerPoint, tc );
	sm[2] = shadowmap_[2].Sample( samplerPoint, tc );
	sm[3] = shadowmap_[3].Sample( samplerPoint, tc );

	float biases[4] = { 0.00005, 0.0001, 0.0005, 0.00085 };
	if( input.lightViewPos[sliceIndex].z / input.lightViewPos[sliceIndex].w > sm[ sliceIndex ] + biases[sliceIndex] )
	{
		// return float4( 1.0, 0.5, 0.1, 1.0 );
		nDotL = 0;
	}

	float3 texcoord;
	texcoord.xy = input.texcoord;
	texcoord.z = 0.0;

	float4 color[4];
	color[0] = blockTexture[0].Sample( samplerFiltered, input.texcoord );
	color[1] = blockTexture[1].Sample( samplerFiltered, input.texcoord );
	color[2] = blockTexture[2].Sample( samplerFiltered, input.texcoord );
	color[3] = blockTexture[3].Sample( samplerFiltered, input.texcoord );

	float4 fogColor = float4( 0.5f, 0.5f, 0.5f, 1.0f );

	float fogFactor = input.linearDepth.x;

	float4 finalColor = /*color[ input.texID ] * */nDotL * sunColorTex * 0.5 +
						/*color[ input.texID ] * */ao * ambientColorTex * 0.7;

	float4 fragmentColor = fogFactor*fogColor + ( 1 - fogFactor )*finalColor;
	// return fragmentColor*0.9 + cascadeColors[ sliceIndex ]*0.1;
	return fragmentColor;
}

bool IsPointInsideProjectedVolume( float3 P )
{
	return saturate( abs( P.x ) ) == abs( P.x ) &&
	       saturate( abs( P.y ) ) == abs( P.y ) &&
	       saturate( P.z ) 		  == P.z;
}
