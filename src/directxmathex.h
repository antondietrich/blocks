#ifndef __DIRECTX_MATH_EX__
#define __DIRECTX_MATH_EX__

//******************************************************************
// Provide math functions to work directly with XMFLOAT types.
// Most likely slower than vector instructuins;
// intended use is for prototyping / not performance-critical code.
//******************************************************************

#include <DirectXMath.h>

// Two-component float

inline DirectX::XMFLOAT2 operator+( DirectX::XMFLOAT2 A, float c )
{
	return DirectX::XMFLOAT2( A.x + c, A.y + c );
}

inline DirectX::XMFLOAT2 operator-( DirectX::XMFLOAT2 A, float c )
{
	return DirectX::XMFLOAT2( A.x - c, A.y - c );
}

inline DirectX::XMFLOAT2 operator+( DirectX::XMFLOAT2 A, DirectX::XMFLOAT2 B )
{
	return DirectX::XMFLOAT2( A.x + B.x, A.y + B.y );
}

inline DirectX::XMFLOAT2 operator-( DirectX::XMFLOAT2 A, DirectX::XMFLOAT2 B )
{
	return DirectX::XMFLOAT2( A.x - B.x, A.y - B.y );
}

inline DirectX::XMFLOAT2 operator*( DirectX::XMFLOAT2 A, const float c )
{
	return DirectX::XMFLOAT2( A.x * c, A.y * c );
}

inline DirectX::XMFLOAT2 operator/( DirectX::XMFLOAT2 A, const float c )
{
	return DirectX::XMFLOAT2( A.x / c, A.y / c );
}

inline float LengthSq( DirectX::XMFLOAT2 v )
{
	float result = v.x * v.x + v.y * v.y;
	return result;
}

inline float Length( DirectX::XMFLOAT2 v )
{
	float lenSq = LengthSq( v );
	float result = sqrt( lenSq );
	return result;
}

inline float DistanceSq( DirectX::XMFLOAT2 A, DirectX::XMFLOAT2 B )
{
	DirectX::XMFLOAT2 vec = { B.x - A.x, B.y - A.y };
	float result = vec.x * vec.x + vec.y * vec.y;
	return result;
}

inline float Distance2( DirectX::XMFLOAT2 A, DirectX::XMFLOAT2 B )
{
	float distSq = DistanceSq( A, B );
	float result = sqrt( distSq );
	return result;
}


// Three-component float

inline DirectX::XMFLOAT3 operator+( DirectX::XMFLOAT3 A, float c )
{
	return DirectX::XMFLOAT3( A.x + c, A.y + c, A.z + c );
}

inline DirectX::XMFLOAT3 operator-( DirectX::XMFLOAT3 A, float c )
{
	return DirectX::XMFLOAT3( A.x - c, A.y - c, A.z - c );
}

inline DirectX::XMFLOAT3 operator+( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B )
{
	return DirectX::XMFLOAT3( A.x + B.x, A.y + B.y, A.z + B.z );
}

inline DirectX::XMFLOAT3 operator-( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B )
{
	return DirectX::XMFLOAT3( A.x - B.x, A.y - B.y, A.z - B.z );
}

inline DirectX::XMFLOAT3 operator*( DirectX::XMFLOAT3 A, float c )
{
	return DirectX::XMFLOAT3( A.x * c, A.y * c, A.z * c );
}

inline DirectX::XMFLOAT3 operator/( DirectX::XMFLOAT3 A, float c )
{
	return DirectX::XMFLOAT3( A.x / c, A.y / c, A.z / c );
}

inline float LengthSq( DirectX::XMFLOAT3 v )
{
	float result = v.x * v.x + v.y * v.y + v.z * v.z;
	return result;
}

inline float Length( DirectX::XMFLOAT3 v )
{
	float lenSq = LengthSq( v );
	float result = sqrt( lenSq );
	return result;
}

inline DirectX::XMFLOAT3 Normalize( DirectX::XMFLOAT3 v )
{
	float len = Length( v );
	DirectX::XMFLOAT3 result = { v.x / len, v.y / len, v.z / len };
	return result;
}

inline float DistanceSq( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B )
{
	DirectX::XMFLOAT3 vec = { B.x - A.x, B.y - A.y, B.z - A.z };
	float result = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	return result;
}

inline float Distance3( DirectX::XMFLOAT3 A, DirectX::XMFLOAT3 B )
{
	float distSq = DistanceSq( A, B );
	float result = sqrt( distSq );
	return result;
}

#endif
