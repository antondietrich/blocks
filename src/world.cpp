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

/*************************************************/

enum FACE_INDEX
{
	FACE_NEG_Z = 0,
	FACE_POS_X = 4,
	FACE_POS_Z = 8,
	FACE_NEG_X = 12,
	FACE_POS_Y = 16,
	FACE_NEG_Y = 20
};

// void AddFace( BlockVertex *vertexBuffer, int vertexIndex, int blockX, int blockY, int blockZ, FACE_INDEX faceIndex );

BlockVertex *chunkVertexBuffer;

void InitWorldGen()
{
	chunkVertexBuffer = new BlockVertex[ MAX_VERTS_PER_CHUNK_MESH ];
}

struct ChunkHeightField
{
	int height[CHUNK_WIDTH][CHUNK_WIDTH];
};

void GenerateChunkHeightField( ChunkHeightField *heightfield, Chunk *chunk, int x, int z, int scale, int heightScale )
{
	chunk->pos[0] = x;
	chunk->pos[1] = z;

	int absX = x >= 0 ?
				abs( x % scale ) :
				( scale - abs( x % scale ) ) % scale;

	int absZ = z >= 0 ?
				abs( z % scale ) :
				( scale - abs( z % scale ) ) % scale;

	int minX = x - absX;
	int maxX = minX + scale;
	int minZ = z - absZ;
	int maxZ = minZ + scale;

	float noise1 = Noise2D( minX, minZ );
	float noise2 = Noise2D( maxX, minZ );
	float noise3 = Noise2D( minX, maxZ );
	float noise4 = Noise2D( maxX, maxZ );

	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			float xT = (float)( absX * CHUNK_WIDTH + blockX ) / ( CHUNK_WIDTH * scale );
			float zT = (float)( absZ * CHUNK_WIDTH + blockZ ) / ( CHUNK_WIDTH * scale );

			float lerp1 = Lerp( noise1, noise2, xT );
			float lerp2 = Lerp( noise3, noise4, xT );
			float lerp3 = Lerp( lerp1, lerp2, zT );

			int height = (int)(lerp3 * heightScale);

			heightfield->height[blockZ][blockX] = height;
		}
	}
}

void GenerateChunk( Chunk *chunk, int x, int z )
{
	ChunkHeightField heightfield1 = {};
	GenerateChunkHeightField( &heightfield1, chunk, x, z, 1, 10 );
	ChunkHeightField heightfield2 = {};
	GenerateChunkHeightField( &heightfield2, chunk, x, z, 8, 20 );

	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			float height = heightfield1.height[blockZ][blockX] + heightfield2.height[blockZ][blockX];

			for( int blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
			{
				if( blockY < height ) {
					chunk->blocks[blockX][blockY][blockZ] = BT_DIRT;
				}
				else if( blockY == height ) {
					chunk->blocks[blockX][blockY][blockZ] = BT_GRASS;
				}
				else {
					chunk->blocks[blockX][blockY][blockZ] = BT_AIR;
				}
			}
		}
	}
}

void GenerateWorld( World *world )
{
	for( int z = 0; z < CHUNK_CACHE_DIM; z++ )
	{
		for( int x = 0; x < CHUNK_CACHE_DIM; x++ )
		{
			GenerateChunk( &world->chunks[ z * CHUNK_CACHE_DIM + x ], x, z );
		}
	}
}

int ChunkCacheIndexFromChunkPos( uint x, uint z )
{
	unsigned int ux = x + INT_MAX;
	unsigned int uz = z + INT_MAX;
	int cacheX = ux % CHUNK_CACHE_DIM;
	int cacheZ = uz % CHUNK_CACHE_DIM;
	int cacheIndex = cacheZ * CHUNK_CACHE_DIM + cacheX;
	return cacheIndex;
}

int MeshCacheIndexFromChunkPos( uint x, uint z )
{
	unsigned int ux = x + INT_MAX;
	unsigned int uz = z + INT_MAX;
	int cacheX = ux % MESH_CACHE_DIM;
	int cacheZ = uz % MESH_CACHE_DIM;
	int cacheIndex = cacheZ * MESH_CACHE_DIM + cacheX;
	return cacheIndex;
}

enum BLOCK_OFFSET_DIRECTION
{
	BOD_SAM,
	BOD_POS,
	BOD_NEG,

	BOD_COUNT
};

void GetNeighbouringBlocks( BLOCK_TYPE neighbours[][BOD_COUNT][BOD_COUNT],
							int x, int y, int z,
							Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
							Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
							Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ )
{
	BLOCK_OFFSET_DIRECTION yBOD = BOD_SAM;

	for( int yOffset = y - 1; yOffset <= y + 1; yOffset++ )
	{
		if( yOffset == y-1 ) {
			yBOD = BOD_NEG;
		}
		else if( yOffset == y+1 ) {
			yBOD = BOD_POS;
		}
		else {
			yBOD = BOD_SAM;
		}

		if( yOffset == -1 ) // bottom layer
		{
			neighbours[BOD_SAM][BOD_NEG][BOD_SAM] = BT_DIRT;
			neighbours[BOD_SAM][BOD_NEG][BOD_NEG] = BT_DIRT;
			neighbours[BOD_SAM][BOD_NEG][BOD_POS] = BT_DIRT;
			neighbours[BOD_NEG][BOD_NEG][BOD_SAM] = BT_DIRT;
			neighbours[BOD_NEG][BOD_NEG][BOD_NEG] = BT_DIRT;
			neighbours[BOD_NEG][BOD_NEG][BOD_POS] = BT_DIRT;
			neighbours[BOD_POS][BOD_NEG][BOD_SAM] = BT_DIRT;
			neighbours[BOD_POS][BOD_NEG][BOD_NEG] = BT_DIRT;
			neighbours[BOD_POS][BOD_NEG][BOD_POS] = BT_DIRT;
		}
		else if( yOffset == CHUNK_HEIGHT ) // top layer
		{
			neighbours[BOD_SAM][BOD_POS][BOD_SAM] = BT_AIR;
			neighbours[BOD_SAM][BOD_POS][BOD_NEG] = BT_AIR;
			neighbours[BOD_SAM][BOD_POS][BOD_POS] = BT_AIR;
			neighbours[BOD_NEG][BOD_POS][BOD_SAM] = BT_AIR;
			neighbours[BOD_NEG][BOD_POS][BOD_NEG] = BT_AIR;
			neighbours[BOD_NEG][BOD_POS][BOD_POS] = BT_AIR;
			neighbours[BOD_POS][BOD_POS][BOD_SAM] = BT_AIR;
			neighbours[BOD_POS][BOD_POS][BOD_NEG] = BT_AIR;
			neighbours[BOD_POS][BOD_POS][BOD_POS] = BT_AIR;
		}
		else
		{
			// center
			neighbours[BOD_SAM][yBOD][BOD_SAM] = chunk->blocks[x][yOffset][z];

			// sides
			neighbours[BOD_POS][yBOD][BOD_SAM] = x == CHUNK_WIDTH-1	? chunkPosX->blocks[0][yOffset][z]				: chunk->blocks[x+1][yOffset][z];
			neighbours[BOD_NEG][yBOD][BOD_SAM] = x == 0				? chunkNegX->blocks[CHUNK_WIDTH-1][yOffset][z]	: chunk->blocks[x-1][yOffset][z];
			neighbours[BOD_SAM][yBOD][BOD_POS] = z == CHUNK_WIDTH-1	? chunkPosZ->blocks[x][yOffset][0]				: chunk->blocks[x][yOffset][z+1];
			neighbours[BOD_SAM][yBOD][BOD_NEG] = z == 0				? chunkNegZ->blocks[x][yOffset][CHUNK_WIDTH-1]	: chunk->blocks[x][yOffset][z-1];

			// corners
			if( x == CHUNK_WIDTH-1 && z == CHUNK_WIDTH-1 )
			{
				neighbours[BOD_POS][yBOD][BOD_POS] = chunkPosXPosZ->blocks[0][yOffset][0];
			}
			else
			{
				if( x == CHUNK_WIDTH-1 ) {
					neighbours[BOD_POS][yBOD][BOD_POS] = chunkPosX->blocks[0][yOffset][z+1];
				}
				else if( z == CHUNK_WIDTH-1 ) {
					neighbours[BOD_POS][yBOD][BOD_POS] = chunkPosZ->blocks[x+1][yOffset][0];
				}
				else {
					neighbours[BOD_POS][yBOD][BOD_POS] = chunk->blocks[x+1][yOffset][z+1];
				}
			}

			if( x == 0 && z == CHUNK_WIDTH-1 )
			{
				neighbours[BOD_NEG][yBOD][BOD_POS] = chunkNegXPosZ->blocks[CHUNK_WIDTH-1][yOffset][0];
			}
			else
			{
				if( x == 0 ) {
					neighbours[BOD_NEG][yBOD][BOD_POS] = chunkNegX->blocks[CHUNK_WIDTH-1][yOffset][z+1];
				}
				else if( z == CHUNK_WIDTH-1 ) {
					neighbours[BOD_NEG][yBOD][BOD_POS] = chunkPosZ->blocks[x-1][yOffset][0];
				}
				else {
					neighbours[BOD_NEG][yBOD][BOD_POS] = chunk->blocks[x-1][yOffset][z+1];
				}
			}

			if( x == CHUNK_WIDTH-1 && z == 0 )
			{
				neighbours[BOD_POS][yBOD][BOD_NEG] = chunkPosXNegZ->blocks[0][yOffset][CHUNK_WIDTH-1];
			}
			else
			{
				if( x == CHUNK_WIDTH-1 ) {
					neighbours[BOD_POS][yBOD][BOD_NEG] = chunkPosX->blocks[0][yOffset][z-1];
				}
				else if( z == 0 ) {
					neighbours[BOD_POS][yBOD][BOD_NEG] = chunkNegZ->blocks[x+1][yOffset][CHUNK_WIDTH-1];
				}
				else {
					neighbours[BOD_POS][yBOD][BOD_NEG] = chunk->blocks[x+1][yOffset][z-1];
				}
			}

			if( x == 0 && z == 0 )
			{
				neighbours[BOD_NEG][yBOD][BOD_NEG] = chunkNegXNegZ->blocks[CHUNK_WIDTH-1][yOffset][CHUNK_WIDTH-1];
			}
			else
			{
				if( x == 0 ) {
					neighbours[BOD_NEG][yBOD][BOD_NEG] = chunkNegX->blocks[CHUNK_WIDTH-1][yOffset][z-1];
				}
				else if( z == 0 ) {
					neighbours[BOD_NEG][yBOD][BOD_NEG] = chunkNegZ->blocks[x-1][yOffset][CHUNK_WIDTH-1];
				}
				else {
					neighbours[BOD_NEG][yBOD][BOD_NEG] = chunk->blocks[x-1][yOffset][z-1];
				}
			}
		} // else
	} // for yOffset 
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

#define PACK_NORMAL_AND_TEXCOORD( normalIndex, texcoordIndex ) ((normalIndex) << 5) | (texcoordIndex << 3) | 0

BlockVertex block[] = 
{
	// face 1 / -Z
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 0, 0 ) }, // 0
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 0, 1 ) }, // 1
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 0, 3 ) }, // 2
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 0, 2 ) }, // 3
	// face 2 / +X
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 1, 0 ) }, // 3
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 1, 1 ) }, // 2
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 1, 3 ) }, // 4
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 1, 2 ) }, // 5
	// face 3 / +Z
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 2, 0 ) }, // 5
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 2, 1 ) }, // 4
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 2, 3 ) }, // 6
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 2, 2 ) }, // 7
	// face 4 / -X
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 3, 0 ) }, // 7
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 3, 1 ) }, // 6
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 3, 3 ) }, // 1
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 3, 2 ) }, // 0
	// face 5 / +Y
	{ 0, 1, 1, PACK_NORMAL_AND_TEXCOORD( 4, 0 ) }, // 7
	{ 0, 1, 0, PACK_NORMAL_AND_TEXCOORD( 4, 1 ) }, // 0
	{ 1, 1, 0, PACK_NORMAL_AND_TEXCOORD( 4, 3 ) }, // 3
	{ 1, 1, 1, PACK_NORMAL_AND_TEXCOORD( 4, 2 ) }, // 5
	// face 6 / -Y
	{ 0, 0, 0, PACK_NORMAL_AND_TEXCOORD( 5, 0 ) }, // 1
	{ 0, 0, 1, PACK_NORMAL_AND_TEXCOORD( 5, 1 ) }, // 6
	{ 1, 0, 1, PACK_NORMAL_AND_TEXCOORD( 5, 3 ) }, // 4
	{ 1, 0, 0, PACK_NORMAL_AND_TEXCOORD( 5, 2 ) }, // 2
};

void AddFace( BlockVertex *vertexBuffer, int startVertexIndex, uint8 blockX, uint8 blockY, uint8 blockZ, FACE_INDEX faceIndex, uint8 occluded[4] )
{
	vertexBuffer[ startVertexIndex + 0 ] = block[ faceIndex + 0];
		vertexBuffer[ startVertexIndex + 0 ].data[0] += blockX;
		vertexBuffer[ startVertexIndex + 0 ].data[1] += blockY;
		vertexBuffer[ startVertexIndex + 0 ].data[2] += blockZ;
		vertexBuffer[ startVertexIndex + 0 ].data[3] |= ( occluded[0] << 1 );
	vertexBuffer[ startVertexIndex + 1 ] = block[ faceIndex + 1];
		vertexBuffer[ startVertexIndex + 1 ].data[0] += blockX;
		vertexBuffer[ startVertexIndex + 1 ].data[1] += blockY;
		vertexBuffer[ startVertexIndex + 1 ].data[2] += blockZ;
		vertexBuffer[ startVertexIndex + 1 ].data[3] |= ( occluded[1] << 1 );
	vertexBuffer[ startVertexIndex + 4 ] = block[ faceIndex + 2];
		vertexBuffer[ startVertexIndex + 4 ].data[0] += blockX;
		vertexBuffer[ startVertexIndex + 4 ].data[1] += blockY;
		vertexBuffer[ startVertexIndex + 4 ].data[2] += blockZ;
		vertexBuffer[ startVertexIndex + 4 ].data[3] |= ( occluded[2] << 1 );
	vertexBuffer[ startVertexIndex + 5 ] = block[ faceIndex + 3];
		vertexBuffer[ startVertexIndex + 5 ].data[0] += blockX;
		vertexBuffer[ startVertexIndex + 5 ].data[1] += blockY;
		vertexBuffer[ startVertexIndex + 5 ].data[2] += blockZ;
		vertexBuffer[ startVertexIndex + 5 ].data[3] |= ( occluded[3] << 1 );

	if( occluded[0] + occluded[2] <= occluded[1] + occluded[3] ) // normal quad
	{
		vertexBuffer[ startVertexIndex + 2 ] = vertexBuffer[ startVertexIndex + 4 ];
		vertexBuffer[ startVertexIndex + 3 ] = vertexBuffer[ startVertexIndex + 0 ];
	}
	else // flipped quad
	{
		vertexBuffer[ startVertexIndex + 2 ] = vertexBuffer[ startVertexIndex + 5 ];
		vertexBuffer[ startVertexIndex + 3 ] = vertexBuffer[ startVertexIndex + 1 ];
	}
}

// TODO: rotate face's inner edge based on occlusion direction (interpolation bugs)

uint8 VertexAO( BLOCK_TYPE side1, BLOCK_TYPE side2, BLOCK_TYPE corner )
{
	if( side1 != BT_AIR && side2 != BT_AIR ) {
		return 3;
	}
	uint8 a1 = side1 == BT_AIR ? 0 : 1;
	uint8 a2 = side2 == BT_AIR ? 0 : 1;
	uint8 a3 = corner == BT_AIR ? 0 : 1;

	return a1 + a2 + a3;
}

int GenerateChunkMesh( ChunkMesh *chunkMesh, Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
											 Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
											 Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ )
{
	int vertexIndex = 0;
	for( uint8 blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
	{
		for( uint8 blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			for( uint8 blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
			{
				if( chunk->blocks[blockX][blockY][blockZ] == BT_AIR ) {
					continue;
				}

				// get neighbouring blocks
				BLOCK_TYPE neighbours[BOD_COUNT][BOD_COUNT][BOD_COUNT];
				GetNeighbouringBlocks( neighbours, 
									   blockX, blockY, blockZ,
									   chunkNegXPosZ,	chunkPosZ,	chunkPosXPosZ,
									   chunkNegX,		chunk,		chunkPosX,
									   chunkNegXNegZ,	chunkNegZ,	chunkPosXNegZ );

//				for( FACE_INDEX faceDir = 0; faceDir < 6; faceDir++ )
//				{
//
//				}

				uint8 occlusion[] = { 0, 0, 0, 0 };

				// X+
				if( neighbours[BOD_POS][BOD_SAM][BOD_SAM] == BT_AIR )
				{
					occlusion[0] = VertexAO( neighbours[BOD_POS][BOD_POS][BOD_SAM],
											neighbours[BOD_POS][BOD_SAM][BOD_NEG], 
											neighbours[BOD_POS][BOD_POS][BOD_NEG] );
					occlusion[1] = VertexAO( neighbours[BOD_POS][BOD_NEG][BOD_SAM],
											neighbours[BOD_POS][BOD_SAM][BOD_NEG], 
											neighbours[BOD_POS][BOD_NEG][BOD_NEG] );
					occlusion[2] = VertexAO( neighbours[BOD_POS][BOD_NEG][BOD_SAM],
											neighbours[BOD_POS][BOD_SAM][BOD_POS], 
											neighbours[BOD_POS][BOD_NEG][BOD_POS] );
					occlusion[3] = VertexAO( neighbours[BOD_POS][BOD_POS][BOD_SAM],
											neighbours[BOD_POS][BOD_SAM][BOD_POS], 
											neighbours[BOD_POS][BOD_POS][BOD_POS] );
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_X, occlusion );
					vertexIndex += VERTS_PER_FACE;
				}

				// X-
				if( neighbours[BOD_NEG][BOD_SAM][BOD_SAM] == BT_AIR )
				{
					occlusion[0] = VertexAO( neighbours[BOD_NEG][BOD_POS][BOD_SAM],
											 neighbours[BOD_NEG][BOD_SAM][BOD_POS],
											 neighbours[BOD_NEG][BOD_POS][BOD_POS] );
					occlusion[3] = VertexAO( neighbours[BOD_NEG][BOD_POS][BOD_SAM],
											 neighbours[BOD_NEG][BOD_SAM][BOD_NEG],
											 neighbours[BOD_NEG][BOD_POS][BOD_NEG] );
					occlusion[2] = VertexAO( neighbours[BOD_NEG][BOD_NEG][BOD_SAM],
											 neighbours[BOD_NEG][BOD_SAM][BOD_NEG],
											 neighbours[BOD_NEG][BOD_NEG][BOD_NEG] );
					occlusion[1] = VertexAO( neighbours[BOD_NEG][BOD_NEG][BOD_SAM],
											 neighbours[BOD_NEG][BOD_SAM][BOD_POS],
											 neighbours[BOD_NEG][BOD_NEG][BOD_POS] );
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_X, occlusion );
					vertexIndex += VERTS_PER_FACE;
				}

				// Z+
				if( neighbours[BOD_SAM][BOD_SAM][BOD_POS] == BT_AIR )
				{
					occlusion[0] = VertexAO( neighbours[BOD_POS][BOD_SAM][BOD_POS],
											 neighbours[BOD_SAM][BOD_POS][BOD_POS],
											 neighbours[BOD_POS][BOD_POS][BOD_POS] );
					occlusion[1] = VertexAO( neighbours[BOD_POS][BOD_SAM][BOD_POS],
											 neighbours[BOD_SAM][BOD_NEG][BOD_POS],
											 neighbours[BOD_POS][BOD_NEG][BOD_POS] );
					occlusion[2] = VertexAO( neighbours[BOD_NEG][BOD_SAM][BOD_POS],
											 neighbours[BOD_SAM][BOD_NEG][BOD_POS],
											 neighbours[BOD_NEG][BOD_NEG][BOD_POS] );
					occlusion[3] = VertexAO( neighbours[BOD_NEG][BOD_SAM][BOD_POS],
											 neighbours[BOD_SAM][BOD_POS][BOD_POS],
											 neighbours[BOD_NEG][BOD_POS][BOD_POS] );
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Z, occlusion );
					vertexIndex += VERTS_PER_FACE;
				}

				// Z-
				if( neighbours[BOD_SAM][BOD_SAM][BOD_NEG] == BT_AIR )
				{
					occlusion[0] = VertexAO( neighbours[BOD_NEG][BOD_SAM][BOD_NEG],
											 neighbours[BOD_SAM][BOD_POS][BOD_NEG],
											 neighbours[BOD_NEG][BOD_POS][BOD_NEG] );
					occlusion[1] = VertexAO( neighbours[BOD_NEG][BOD_SAM][BOD_NEG],
											 neighbours[BOD_SAM][BOD_NEG][BOD_NEG],
											 neighbours[BOD_NEG][BOD_NEG][BOD_NEG] );
					occlusion[2] = VertexAO( neighbours[BOD_POS][BOD_SAM][BOD_NEG],
											 neighbours[BOD_SAM][BOD_NEG][BOD_NEG],
											 neighbours[BOD_POS][BOD_NEG][BOD_NEG] );
					occlusion[3] = VertexAO( neighbours[BOD_POS][BOD_SAM][BOD_NEG],
											 neighbours[BOD_SAM][BOD_POS][BOD_NEG],
											 neighbours[BOD_POS][BOD_POS][BOD_NEG] );
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_Z, occlusion );
					vertexIndex += VERTS_PER_FACE;
				}

				// Y+
				if( neighbours[BOD_SAM][BOD_POS][BOD_SAM] == BT_AIR )
				{
					occlusion[0] = VertexAO( neighbours[BOD_NEG][BOD_POS][BOD_SAM],
											 neighbours[BOD_SAM][BOD_POS][BOD_POS],
											 neighbours[BOD_NEG][BOD_POS][BOD_POS] );
					occlusion[1] = VertexAO( neighbours[BOD_NEG][BOD_POS][BOD_SAM],
											 neighbours[BOD_SAM][BOD_POS][BOD_NEG],
											 neighbours[BOD_NEG][BOD_POS][BOD_NEG] );
					occlusion[2] = VertexAO( neighbours[BOD_POS][BOD_POS][BOD_SAM],
											 neighbours[BOD_SAM][BOD_POS][BOD_NEG],
											 neighbours[BOD_POS][BOD_POS][BOD_NEG] );
					occlusion[3] = VertexAO( neighbours[BOD_POS][BOD_POS][BOD_SAM],
											 neighbours[BOD_SAM][BOD_POS][BOD_POS],
											 neighbours[BOD_POS][BOD_POS][BOD_POS] );
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Y, occlusion );
					vertexIndex += VERTS_PER_FACE;
				}

				// Y-
				occlusion[0] = 0;
				occlusion[1] = 0;
				occlusion[2] = 0;
				occlusion[3] = 0;
				if( neighbours[BOD_SAM][BOD_NEG][BOD_SAM] == BT_AIR )
				{
					// TODO: add downward face
				}
			}
		}
	}

	chunkMesh->vertices = new BlockVertex[ vertexIndex ];
	memcpy( chunkMesh->vertices, chunkVertexBuffer, sizeof( BlockVertex ) * vertexIndex );
	chunkMesh->size = vertexIndex;

	return vertexIndex;
}

}