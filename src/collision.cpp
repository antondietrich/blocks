#include "collision.h"

namespace Blocks
{

using namespace DirectX;

bool TestIntersection( Segment seg, AABB bound )
{
	if( seg.B.x > bound.min.x &&
		seg.B.y > bound.min.y &&
		seg.B.z > bound.min.z &&
		seg.B.x < bound.max.x &&
		seg.B.y < bound.max.y &&
		seg.B.z < bound.max.z )
	{
		return true;
	}
	return false;
}

float max( float a, float b )
{
	if( b > a)
	{
		return b;
	}
	return a;
}

float min( float a, float b )
{
	if( b < a )
	{
		return b;
	}
	return a;
}

void swap( float &a, float &b )
{
	float tmp = a;
	a = b;
	b = tmp;
}

#define ARRAY_SIZE( a ) sizeof( a ) / sizeof( a[0] )

//
// Test for an intersection between a line segment and an AABB using SAT
//
bool TestIntersection( Line line, AABB bound )
{
	XMVECTOR rayStart = XMLoadFloat3( &line.p );
	XMVECTOR rayDir = XMLoadFloat3( &line.d );

	XMVECTOR boundVertices[8];
	boundVertices[0] = XMVectorSet( bound.min.x, bound.min.y, bound.min.z, 0.0f );
	boundVertices[1] = XMVectorSet( bound.min.x, bound.min.y, bound.max.z, 0.0f );
	boundVertices[2] = XMVectorSet( bound.min.x, bound.max.y, bound.min.z, 0.0f );
	boundVertices[3] = XMVectorSet( bound.min.x, bound.max.y, bound.max.z, 0.0f );
	boundVertices[4] = XMVectorSet( bound.max.x, bound.min.y, bound.min.z, 0.0f );
	boundVertices[5] = XMVectorSet( bound.max.x, bound.min.y, bound.max.z, 0.0f );
	boundVertices[6] = XMVectorSet( bound.max.x, bound.max.y, bound.min.z, 0.0f );
	boundVertices[7] = XMVectorSet( bound.max.x, bound.max.y, bound.max.z, 0.0f );

	XMVECTOR separatingAxes[6];
	separatingAxes[0] = XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f );
	separatingAxes[1] = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	separatingAxes[2] = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );
	separatingAxes[3] = XMVector3Normalize( XMVector3Cross( rayDir, separatingAxes[0] ) );
	separatingAxes[4] = XMVector3Normalize( XMVector3Cross( rayDir, separatingAxes[1] ) );
	separatingAxes[5] = XMVector3Normalize( XMVector3Cross( rayDir, separatingAxes[2] ) );

	for( int axisIndex = 0; axisIndex < ARRAY_SIZE( separatingAxes ); axisIndex++ )
	{
		float rayStartProjectionLength = XMVectorGetX( XMVector3Dot( separatingAxes[axisIndex], rayStart ) );
		float rayEndProjectionLength = rayStartProjectionLength + XMVectorGetX( XMVector3Dot( separatingAxes[axisIndex], rayDir ) );

		float rayMin = min( rayStartProjectionLength, rayEndProjectionLength );
		float rayMax = max( rayStartProjectionLength, rayEndProjectionLength );

		float boundProjections[8];
		for( int boundVertexIndex = 0; boundVertexIndex < 8; boundVertexIndex++ )
		{
			boundProjections[boundVertexIndex] = XMVectorGetX( XMVector3Dot( separatingAxes[axisIndex], boundVertices[boundVertexIndex] ) );
		}

		// TODO: replace bubble sort with smth better?
		int numSwaps = 1;
		while( numSwaps != 0 )
		{
			numSwaps = 0;
			for( int i = 0; i < 7; i++ )
			{
				if( boundProjections[i] > boundProjections[ i + 1 ] )
				{
					swap( boundProjections[i], boundProjections[ i + 1 ] );
					++numSwaps;
				}
			}
		}

		float boundMin = boundProjections[ 0 ];
		float boundMax = boundProjections[ 7 ];

		if( rayMax < boundMin ||
			rayMin > boundMax )
		{
			return false;
		}

	}

	return true;
}

float DistanceSq( XMFLOAT3 A, XMFLOAT3 B )
{
	XMFLOAT3 vec = { B.x - A.x, B.y - A.y, B.z - A.z };
	float result = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	return result;
}


} // namespace Blocks