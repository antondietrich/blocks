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
	DirectX::XMFLOAT3 p;
	DirectX::XMFLOAT3 d;
	float length;
};

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};

}

#endif // __BLOCKS_COLLISION__