float Lambert( float4 normal, float4 negLightDir )
{
	return saturate( dot( normal, negLightDir ) );
}
