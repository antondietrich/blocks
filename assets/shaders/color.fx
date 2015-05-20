struct VS_Input
{
	float3 pos : POSITION;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
};

PS_Input VSMain( VS_Input input )
{
	PS_Input output;
	output.pos.xyz = input.pos;
	output.pos.w = 1.0f;
	return output;
}

float4 PSMain( PS_Input input ) : SV_TARGET
{
	return float4( 1.0f, 1.0f, 0.0f, 1.0f );
}