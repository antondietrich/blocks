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

	XMStoreFloat4( &p, vn );
	p.w = XMVectorGetX( vd );
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


Frustum::Frustum( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B, DirectX::XMFLOAT3 C, DirectX::XMFLOAT3 D,
		 DirectX::XMFLOAT3 E, DirectX::XMFLOAT3 F, DirectX::XMFLOAT3 G, DirectX::XMFLOAT3 H )
{
	corners[0] = A;
	corners[1] = B;
	corners[2] = C;
	corners[3] = D;
	corners[4] = E;
	corners[5] = F;
	corners[6] = G;
	corners[7] = H;

	planes[0] = Plane( corners[ 0 ], corners[ 3 ], corners[ 4 ] );
	planes[1] = Plane( corners[ 0 ], corners[ 4 ], corners[ 1 ] );
	planes[2] = Plane( corners[ 1 ], corners[ 5 ], corners[ 2 ] );
	planes[3] = Plane( corners[ 2 ], corners[ 6 ], corners[ 3 ] );
	planes[4] = Plane( corners[ 0 ], corners[ 1 ], corners[ 3 ] );
	planes[5] = Plane( corners[ 5 ], corners[ 4 ], corners[ 6 ] );
}

Frustum CalculatePerspectiveFrustum( const DirectX::XMFLOAT3 &eyePos,
									 const DirectX::XMFLOAT3 &front,
									 const DirectX::XMFLOAT3 &right,
									 const DirectX::XMFLOAT3 &up,
									 const float near,
									 const float far,
									 const float hFovDeg,
									 const float aspect )
{
	XMFLOAT3 frustumA;
	XMFLOAT3 frustumB;
	XMFLOAT3 frustumC;
	XMFLOAT3 frustumD;
	XMFLOAT3 frustumE;
	XMFLOAT3 frustumF;
	XMFLOAT3 frustumG;
	XMFLOAT3 frustumH;

	float hFovRad = XMConvertToRadians( hFovDeg );
	float vFovRad = hFovRad / aspect;
	float halfWidthNear  = near * tan( hFovRad*0.5f );
	float halfHeightNear = near * tan( vFovRad*0.5f );
	float halfWidthFar   = far * tan( hFovRad*0.5f );
	float halfHeightFar  = far * tan( vFovRad*0.5f );

	XMVECTOR vEyePos = XMLoadFloat3( &eyePos );
	XMVECTOR vFront = XMLoadFloat3( &front );
	XMVECTOR vRight = XMLoadFloat3( &right );
	XMVECTOR vUp = XMLoadFloat3( &up );

	// near plane corners
	XMStoreFloat3( &frustumA, vEyePos + vFront*near - vRight*halfWidthNear - vUp*halfHeightNear );
	XMStoreFloat3( &frustumB, vEyePos + vFront*near + vRight*halfWidthNear - vUp*halfHeightNear );
	XMStoreFloat3( &frustumC, vEyePos + vFront*near + vRight*halfWidthNear + vUp*halfHeightNear );
	XMStoreFloat3( &frustumD, vEyePos + vFront*near - vRight*halfWidthNear + vUp*halfHeightNear );
	// far plane corners
	XMStoreFloat3( &frustumE, vEyePos + vFront*far - vRight*halfWidthFar - vUp*halfHeightFar );
	XMStoreFloat3( &frustumF, vEyePos + vFront*far + vRight*halfWidthFar - vUp*halfHeightFar );
	XMStoreFloat3( &frustumG, vEyePos + vFront*far + vRight*halfWidthFar + vUp*halfHeightFar );
	XMStoreFloat3( &frustumH, vEyePos + vFront*far - vRight*halfWidthFar + vUp*halfHeightFar );

	Frustum frustum = Frustum( frustumA, frustumB, frustumC, frustumD, frustumE, frustumF, frustumG, frustumH );

	return frustum;
}

Frustum ComputeOrthoFrustum( const DirectX::XMFLOAT3 &eyePos,
							 const DirectX::XMFLOAT3 &front,
							 const DirectX::XMFLOAT3 &right,
							 const DirectX::XMFLOAT3 &up,
							 const float width,
							 const float height,
							 const float near,
							 const float far )
{
	XMFLOAT3 frustumA;
	XMFLOAT3 frustumB;
	XMFLOAT3 frustumC;
	XMFLOAT3 frustumD;
	XMFLOAT3 frustumE;
	XMFLOAT3 frustumF;
	XMFLOAT3 frustumG;
	XMFLOAT3 frustumH;

	XMVECTOR vEyePos = XMLoadFloat3( &eyePos );
	XMVECTOR vFront = XMLoadFloat3( &front );
	XMVECTOR vRight = XMLoadFloat3( &right );
	XMVECTOR vUp = XMLoadFloat3( &up );

	XMStoreFloat3( &frustumA, vEyePos + vFront*near - vRight*width*0.5f - vUp*height*0.5f );
	XMStoreFloat3( &frustumB, vEyePos + vFront*near + vRight*width*0.5f - vUp*height*0.5f );
	XMStoreFloat3( &frustumC, vEyePos + vFront*near + vRight*width*0.5f + vUp*height*0.5f );
	XMStoreFloat3( &frustumD, vEyePos + vFront*near - vRight*width*0.5f + vUp*height*0.5f );
	XMStoreFloat3( &frustumE, vEyePos + vFront*far  - vRight*width*0.5f - vUp*height*0.5f );
	XMStoreFloat3( &frustumF, vEyePos + vFront*far  + vRight*width*0.5f - vUp*height*0.5f );
	XMStoreFloat3( &frustumG, vEyePos + vFront*far  + vRight*width*0.5f + vUp*height*0.5f );
	XMStoreFloat3( &frustumH, vEyePos + vFront*far  - vRight*width*0.5f + vUp*height*0.5f );

	Frustum frustum = Frustum( frustumA, frustumB, frustumC, frustumD,
								  frustumE, frustumF, frustumG, frustumH );

	return frustum;
}

bool IsFrustumCulled( const Frustum &frustum, const AABB &aabb )
{
	for( int planeIndex = 0; planeIndex < 6; planeIndex++ )
	{
		if( TestIntersectionFast( frustum.planes[ planeIndex ], aabb ) == OUTSIDE )
		{
			return true;
		}
	}
	return false;
}

// Real-time Rendering:755
// TODO: https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
INTERSECTION TestIntersectionFast( const Plane &P, const AABB &bound )
{
#if 1
	XMVECTOR boundCenter = XMVectorSet( (bound.max.x + bound.min.x) * 0.5f,
										(bound.max.y + bound.min.y) * 0.5f,
										(bound.max.z + bound.min.z) * 0.5f,
										1.0f );

	XMVECTOR halfDiagonal = XMVectorSet( (bound.max.x - bound.min.x) * 0.5f,
										 (bound.max.y - bound.min.y) * 0.5f,
										 (bound.max.z - bound.min.z) * 0.5f,
										 0.0f );

	XMVECTOR normal = XMLoadFloat4( &P.p );
	XMVECTOR normalAbs = XMVectorAbs( normal );

	XMVECTOR extent = XMVector3Dot( halfDiagonal, normalAbs );
	XMVECTOR distancePlane = XMVector4Dot( boundCenter, normal );

	XMVECTOR zero = XMVectorZero();

	if( XMVectorGetX( XMVectorGreaterOrEqual( distancePlane + extent, zero ) ) ) return INSIDE;
	if( XMVectorGetX( XMVectorLessOrEqual( distancePlane + extent, zero ) ) ) return OUTSIDE;
	return INTERSECTS;
#else
	XMFLOAT4 boundCenter = { (bound.max.x + bound.min.x) * 0.5f,
							 (bound.max.y + bound.min.y) * 0.5f,
							 (bound.max.z + bound.min.z) * 0.5f,
							 1.0f };

	XMFLOAT3 halfDiagonal = { (bound.max.x - bound.min.x) * 0.5f,
							  (bound.max.y - bound.min.y) * 0.5f,
							  (bound.max.z - bound.min.z) * 0.5f };

	float extent = halfDiagonal.x * abs( P.p.x ) + halfDiagonal.y * abs( P.p.y ) + halfDiagonal.z * abs( P.p.z );

	float distancePlane = Distance( boundCenter, P );

	if( distancePlane - extent > 0 ) return INSIDE;
	if( distancePlane + extent < 0 ) return OUTSIDE;
	return INTERSECTS;

#endif
}

#if 0
INTERSECTION TestIntersection( Plane p, AABB bound )
{
#if 0
	XMFLOAT4 points[8];
	points[0] = { bound.min.x, bound.min.y, bound.min.z, 1.0f };
	points[1] = { bound.min.x, bound.min.y, bound.max.z, 1.0f };
	points[2] = { bound.min.x, bound.max.y, bound.min.z, 1.0f };
	points[3] = { bound.min.x, bound.max.y, bound.max.z, 1.0f };
	points[4] = { bound.max.x, bound.min.y, bound.min.z, 1.0f };
	points[5] = { bound.max.x, bound.min.y, bound.max.z, 1.0f };
	points[6] = { bound.max.x, bound.max.y, bound.min.z, 1.0f };
	points[7] = { bound.max.x, bound.max.y, bound.min.z, 1.0f };
#else
	XMVECTOR plane = XMLoadFloat4( &p.p );
	XMVECTOR points[8];
	points[0] = XMVectorSet( bound.min.x, bound.min.y, bound.min.z, 1.0f );
	points[1] = XMVectorSet( bound.min.x, bound.min.y, bound.max.z, 1.0f );
	points[2] = XMVectorSet( bound.min.x, bound.max.y, bound.min.z, 1.0f );
	points[3] = XMVectorSet( bound.min.x, bound.max.y, bound.max.z, 1.0f );
	points[4] = XMVectorSet( bound.max.x, bound.min.y, bound.min.z, 1.0f );
	points[5] = XMVectorSet( bound.max.x, bound.min.y, bound.max.z, 1.0f );
	points[6] = XMVectorSet( bound.max.x, bound.max.y, bound.min.z, 1.0f );
	points[7] = XMVectorSet( bound.max.x, bound.max.y, bound.min.z, 1.0f );
#endif

	int result = 0;

#if 0
	for( int i = 0; i < 8; i++ )
	{
		// float distance = Distance( points[i], p );
		// float distance = XMVector4Dot( plane, points[0] );
		XMVECTOR distance = XMVector4Dot( plane, points[i] );
		// float distance = p.p.x*points[i].x + p.p.y*points[i].y + p.p.z*points[i].z + p.p.w;
		// if( distance >= 0 )
		if( XMVectorGetX( XMVectorGreaterOrEqual( distance, zero ) ) )
		{
			result++;
		}
		else
		{
			result--;
		}
	}
#else
	XMVECTOR distance[8];
	distance[0] = XMVector4Dot( plane, points[0] );
	distance[1] = XMVector4Dot( plane, points[1] );
	distance[2] = XMVector4Dot( plane, points[2] );
	distance[3] = XMVector4Dot( plane, points[3] );
	distance[4] = XMVector4Dot( plane, points[4] );
	distance[5] = XMVector4Dot( plane, points[5] );
	distance[6] = XMVector4Dot( plane, points[6] );
	distance[7] = XMVector4Dot( plane, points[7] );

	XMVECTOR zero = XMVectorZero();
	XMVectorGetX( XMVectorGreaterOrEqual( distance[0], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[1], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[2], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[3], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[4], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[5], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[6], zero ) ) ? ++result : --result;
	XMVectorGetX( XMVectorGreaterOrEqual( distance[7], zero ) ) ? ++result : --result;
#endif

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
#endif

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

} // namespace Blocks
