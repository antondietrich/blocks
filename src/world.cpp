#include "world.h"

namespace Blocks
{

// http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
//
// returns a random float in the 0.0 - 1.0 range
//
float Noise1( int x )			 
{
    x = (x<<13) ^ x;
    float noise = 1.0f - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    return noise / 2.0f + 0.5f;
}

float Noise2D( int x, int y )
{
	int input = ( x*383 ) + y;
	return Noise1( input );
}

float Lerp( float a, float b, float t )
{
	return a * ( 1 - t ) + b * t;
}

float InterpolatedNoise( float x, float y )
{
	int intX = (int)x;
	int intY = (int)y;

	float fracX = x - intX;
	float fracY = y - intY;

	float n1 = Noise2D( intX, intY );
	float n2 = Noise2D( intX+1, intY );
	float n3 = Noise2D( intX, intY+1 );
	float n4 = Noise2D( intX+1, intY+1 );

	float i1 = Lerp( n1, n2, fracX );
	float i2 = Lerp( n3, n4, fracX );

	float i = Lerp( i1, i2, fracY );

	return i;
}

void GenerateWorld( World *world )
{
	srand( 12345 );
	for( int z = 0; z < 32; z++ )
	{
		for( int x = 0; x < 32; x++ )
		{
			for( int blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
			{
				for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
				{
					for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
					{
						float scale = 0.125f;
						scale = 0.0625f / 2.0f;
						int height = (int)( InterpolatedNoise( ( x * CHUNK_WIDTH + blockX ) * scale, ( z * CHUNK_WIDTH + blockZ ) * scale ) * 40 );

						if( blockY < height ) {
							world->chunks[x][z].blocks[blockX][blockY][blockZ] = BT_DIRT;
						}
						else if( blockY == height ) {
							world->chunks[x][z].blocks[blockX][blockY][blockZ] = BT_GRASS;
						}
						else {
							world->chunks[x][z].blocks[blockX][blockY][blockZ] = BT_AIR;
						}
					}
				}
			}
		}
	}
}

int MeshCacheIndexFromChunkPos( unsigned int x, unsigned int z )
{
	int cacheX = x % VISIBLE_CHUNKS_RADIUS;
	int cacheZ = z % VISIBLE_CHUNKS_RADIUS;
	int cacheIndex = cacheZ * VISIBLE_CHUNKS_RADIUS + cacheX;
	return cacheIndex;
}

//cbData.normals[0] = {  0.0f,  0.0f, -1.0f, 0.0f }; // -Z
//cbData.normals[1] = {  1.0f,  0.0f,  0.0f, 0.0f }; // +X
//cbData.normals[2] = {  0.0f,  0.0f,  1.0f, 0.0f }; // +Z
//cbData.normals[3] = { -1.0f,  0.0f,  0.0f, 0.0f }; // -X
//cbData.normals[4] = {  0.0f,  1.0f,  0.0f, 0.0f }; // +Y
//cbData.normals[5] = {  0.0f, -1.0f,  0.0f, 0.0f }; // -Y
//
//cbData.texcoords[0] = { 0.0f, 0.0f, 0.0f, 0.0f }; // top left
//cbData.texcoords[1] = { 0.0f, 1.0f, 0.0f, 0.0f }; // bottom left
//cbData.texcoords[2] = { 1.0f, 0.0f, 0.0f, 0.0f }; // top right
//cbData.texcoords[3] = { 1.0f, 1.0f, 0.0f, 0.0f }; // bottom right

#define PACK_NORMAL_AND_TEXCOORD( normalIndex, texcoordIndex ) ((normalIndex) << 4) | (texcoordIndex)

BlockVertex block[] = 
{
	// face 1 / -Z
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 0, 0 ) }, // 0
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 0, 1 ) }, // 1
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 0, 3 ) }, // 2
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 0, 0 ) }, // 0
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 0, 3 ) }, // 2
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 0, 2 ) }, // 3
	// face 2 / +X
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 1, 0 ) }, // 3
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 1, 1 ) }, // 2
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 1, 3 ) }, // 4
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 1, 0 ) }, // 3
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 1, 3 ) }, // 4
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 1, 2 ) }, // 5
	// face 3 / +Z
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 2, 0 ) }, // 5
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 2, 1 ) }, // 4
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 2, 3 ) }, // 6
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 2, 0 ) }, // 5
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 2, 3 ) }, // 6
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 2, 2 ) }, // 7
	// face 4 / -X
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 3, 0 ) }, // 7
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 3, 1 ) }, // 6
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 3, 3 ) }, // 1
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 3, 0 ) }, // 7
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 3, 3 ) }, // 1
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 3, 2 ) }, // 0
	// face 5 / +Y
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 4, 0 ) }, // 7
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 4, 1 ) }, // 0
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 4, 3 ) }, // 3
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 4, 0 ) }, // 7
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 4, 3 ) }, // 3
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 4, 2 ) }, // 5
	// face 6 / -Y
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 5, 0 ) }, // 1
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 5, 1 ) }, // 6
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 5, 3 ) }, // 4
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 5, 0 ) }, // 1
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 5, 3 ) }, // 4
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 5, 2 ) }, // 2
};

void AddFace( BlockVertex *vertexBuffer, int vertexIndex, int blockX, int blockY, int blockZ, FACE_INDEX faceIndex )
{
	memcpy( &vertexBuffer[ vertexIndex ],
			&block[ faceIndex ],
			sizeof( BlockVertex ) * VERTS_PER_FACE );

	for( int i = 0; i < VERTS_PER_FACE; i++ )
	{
		vertexBuffer[ vertexIndex + i ].data[0] += blockX;
		vertexBuffer[ vertexIndex + i ].data[1] += blockY;
		vertexBuffer[ vertexIndex + i ].data[2] += blockZ;
	}
}

}