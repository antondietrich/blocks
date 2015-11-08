#include "collision.h"

namespace Blocks
{

using namespace DirectX;

Plane::Plane( DirectX::XMFLOAT3 p0, DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2 )
{
	XMVECTOR vp0 = XMLoadFloat3( &p0 );
	XMVECTOR vp1 = XMLoadFloat3( &p1 );
	XMVECTOR vp2 = XMLoadFloat3( &p2 );

	XMVECTOR v0 = vp1 - vp0;
	XMVECTOR v1 = vp2 - vp0;

	XMVECTOR vn = XMVector3Normalize( XMVector3Cross( v0, v1 ) );
	XMVECTOR vd = XMVector3Dot( -vn, vp0 );

	XMStoreFloat3( &n, vn );
	d = XMVectorGetX( vd );
}

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

// TODO: https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
INTERSECTION TestIntersection( Plane p, AABB bound )
{
	XMFLOAT3 points[8];
	points[0] = { bound.min.x, bound.min.y, bound.min.z };
	points[1] = { bound.min.x, bound.min.y, bound.max.z };
	points[2] = { bound.min.x, bound.max.y, bound.min.z };
	points[3] = { bound.min.x, bound.max.y, bound.max.z };
	points[4] = { bound.max.x, bound.min.y, bound.min.z };
	points[5] = { bound.max.x, bound.min.y, bound.max.z };
	points[6] = { bound.max.x, bound.max.y, bound.min.z };
	points[7] = { bound.max.x, bound.max.y, bound.min.z };

	int result = 0;

	for( int i = 0; i < 8; i++ )
	{
		if( Distance( points[i], p ) >= 0 )
		{
			result++;
		}
		else if( Distance( points[i], p ) < 0 )
		{
			result--;
		}
	}

	if( result == 8 )
	{
		return INSIDE;
	}
	if( result == -8 )
	{
		return OUTSIDE;
	}
	return INTERSECTS;
}

RayAABBIntersection GetIntersection( Line ray, AABB bound )
{
	RayAABBIntersection result;

	XMVECTOR intersectionDist = XMVectorReplicate( 100.0f );

	XMVECTOR rayN = XMLoadFloat3( &ray.d );
	rayN = XMVector3Normalize( rayN );
	XMVECTOR rayP = XMLoadFloat3( &ray.p );

	XMVECTOR boundMin = XMLoadFloat3( &bound.min );
	XMVECTOR boundMax = XMLoadFloat3( &bound.max );

	XMVECTOR planeN[6];
	XMVECTOR planeP[6];

	planeN[0] = XMVectorSet( -1.0f, 0.0f, 0.0f, 0.0f );
	planeP[0] = XMLoadFloat3( &bound.min );
	planeN[1] = XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f );
	planeP[1] = XMLoadFloat3( &bound.max );
	planeN[2] = XMVectorSet( 0.0f, -1.0f, 0.0f, 0.0f );
	planeP[2] = XMLoadFloat3( &bound.min );
	planeN[3] = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	planeP[3] = XMLoadFloat3( &bound.max );
	planeN[4] = XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f );
	planeP[4] = XMLoadFloat3( &bound.min );
	planeN[5] = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );
	planeP[5] = XMLoadFloat3( &bound.max );

	for( int i = 0; i < 6; ++i )
	{
		XMVECTOR dot = XMVector3Dot( rayN, planeN[ i ] );
		if( XMVector3Equal( dot, XMVectorZero() ) )
		{
			continue;
		}
		XMVECTOR numerator = XMVector3Dot( planeP[i] - rayP, planeN[i] );
		XMVECTOR t = numerator / dot;

		if( XMVector2Less( t, intersectionDist ) )
		{
			XMVECTOR vIntersection = rayP + rayN * t; // piecewise multiplication
			if( XMVector3LessOrEqual( vIntersection, boundMax ) &&
				XMVector3GreaterOrEqual( vIntersection, boundMin ) )
			{
				intersectionDist = t;
				XMStoreFloat3( &result.plane, planeN[i] );
			}
		}
	}

	XMVECTOR vIntersection = rayP + rayN * XMVectorGetX( intersectionDist );
//	XMFLOAT3 result.point;
	XMStoreFloat3( &result.point, vIntersection );

	return result;
}

float LengthSq( XMFLOAT3 v )
{
	float result = v.x * v.x + v.y * v.y + v.z * v.z;
	return result;
}

float Length( XMFLOAT3 v )
{
	float lenSq = LengthSq( v );
	float result = sqrt( lenSq );
	return result;
}

XMFLOAT3 Normalize( XMFLOAT3 v )
{
	float len = Length( v );
	XMFLOAT3 result = { v.x / len, v.y / len, v.z / len };
	return result;
}

float DistanceSq( XMFLOAT3 A, XMFLOAT3 B )
{
	XMFLOAT3 vec = { B.x - A.x, B.y - A.y, B.z - A.z };
	float result = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	return result;
}

float Distance( DirectX::XMFLOAT3 A, Plane P )
{
	float distance = P.n.x*A.x + P.n.y*A.y + P.n.z*A.z + P.d;
	return distance;
}


} // namespace Blocks
