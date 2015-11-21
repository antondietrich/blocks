#ifndef __BLOCKS_COLLISION__
#define __BLOCKS_COLLISION__

#include <DirectXMath.h>

namespace Blocks
{

enum INTERSECTION
{
	OUTSIDE = 0,
	INTERSECTS = 1,
	INSIDE = 1
};

// Collision primitives

struct Line
{
	DirectX::XMFLOAT3 p; // point on the line
	DirectX::XMFLOAT3 d; // unit direction vector
};

struct Segment
{
	DirectX::XMFLOAT3 A;
	DirectX::XMFLOAT3 B;
};

struct Plane
{
	Plane() {};
	Plane( DirectX::XMFLOAT3 p0, DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2 );
	DirectX::XMFLOAT3 n;
	float d;
};

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};



struct Frustum
{
	Frustum() {};
	Frustum( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B, DirectX::XMFLOAT3 C, DirectX::XMFLOAT3 D,
			 DirectX::XMFLOAT3 E, DirectX::XMFLOAT3 F, DirectX::XMFLOAT3 G, DirectX::XMFLOAT3 H );
	~Frustum() {};
	DirectX::XMFLOAT3 corners[8];
	Plane planes[6];
};
bool IsFrustumCulled( const Frustum&, AABB );



struct RayAABBIntersection
{
	DirectX::XMFLOAT3 point;
	DirectX::XMFLOAT3 plane;
};

bool TestIntersection( Segment, AABB );
bool TestIntersection( Line, AABB );
INTERSECTION TestIntersection( Plane, AABB );

RayAABBIntersection GetIntersection( Line, AABB );

float DistanceSq( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B );
float LengthSq( DirectX::XMFLOAT3 v );
float Length( DirectX::XMFLOAT3 v );
DirectX::XMFLOAT3 Normalize( DirectX::XMFLOAT3 v );
float Distance( DirectX::XMFLOAT3 A, Plane P );

} // namespace Blocks

#endif // __BLOCKS_COLLISION__
