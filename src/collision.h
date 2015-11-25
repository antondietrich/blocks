#ifndef __BLOCKS_COLLISION__
#define __BLOCKS_COLLISION__

#include <DirectXMath.h>

namespace Blocks
{

enum INTERSECTION
{
	OUTSIDE = -1,
	INTERSECTS = 0,
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
	DirectX::XMFLOAT4 p;
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
bool IsFrustumCulled( const Frustum&, const AABB& );


struct RayAABBIntersection
{
	DirectX::XMFLOAT3 point;
	DirectX::XMFLOAT3 plane;
};

bool TestIntersection( Segment, AABB );
bool TestIntersection( Line, AABB );
// INTERSECTION TestIntersection( Plane, AABB );
INTERSECTION TestIntersectionFast( const Plane&, const AABB& );

RayAABBIntersection GetIntersection( Line, AABB );

inline float Distance( const DirectX::XMFLOAT4 &A, const Plane &P )
{
	float distance = P.p.x*A.x + P.p.y*A.y + P.p.z*A.z + P.p.w;
	return distance;
}

} // namespace Blocks

#endif // __BLOCKS_COLLISION__
