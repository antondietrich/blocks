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

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};

bool TestIntersection( Segment, AABB );
bool TestIntersection( Line, AABB );

float DistanceSq( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B );

} // namespace Blocks

#endif // __BLOCKS_COLLISION__