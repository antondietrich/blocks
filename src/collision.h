#ifndef __BLOCKS_COLLISION__
#define __BLOCKS_COLLISION__

#include <DirectXMath.h>

namespace Blocks
{

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
	DirectX::XMFLOAT3 p;
	DirectX::XMFLOAT3 n;
};

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};

bool TestIntersection( Segment, AABB );
bool TestIntersection( Line, AABB );

DirectX::XMFLOAT3 GetIntersection( Line, AABB );

float DistanceSq( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B );
float LengthSq( DirectX::XMFLOAT3 v );
float Length( DirectX::XMFLOAT3 v );
DirectX::XMFLOAT3 Normalize( DirectX::XMFLOAT3 v );

} // namespace Blocks

#endif // __BLOCKS_COLLISION__