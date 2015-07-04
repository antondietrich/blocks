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
	FACE_POS_X = 6,
	FACE_POS_Z = 12,
	FACE_NEG_X = 18,
	FACE_POS_Y = 24,
	FACE_NEG_Y = 30
};

// void AddFace( BlockVertex *vertexBuffer, int vertexIndex, int blockX, int blockY, int blockZ, FACE_INDEX faceIndex );

BlockVertex *chunkVertexBuffer;

void InitWorldGen()
{
	chunkVertexBuffer = new BlockVertex[ MAX_VERTS_PER_CHUNK_MESH ];
}

void GenerateWorld( World *world )
{
	// srand( 12345 );
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

#define PACK_NORMAL_AND_TEXCOORD( normalIndex, texcoordIndex ) ((normalIndex) << 5) | (texcoordIndex << 2) | 0

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

void AddFace( BlockVertex *vertexBuffer, int startVertexIndex, uint8_t blockX, uint8_t blockY, uint8_t blockZ, FACE_INDEX faceIndex )
{
	memcpy( &vertexBuffer[ startVertexIndex ],
			&block[ faceIndex ],
			sizeof( BlockVertex ) * VERTS_PER_FACE );

	for( int i = 0; i < VERTS_PER_FACE; i++ )
	{
		vertexBuffer[ startVertexIndex + i ].data[0] += blockX;
		vertexBuffer[ startVertexIndex + i ].data[1] += blockY;
		vertexBuffer[ startVertexIndex + i ].data[2] += blockZ;
	}
}

int GenerateChunkMesh( ChunkMesh *chunkMesh, Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
											 Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
											 Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ )
{
	int vertexIndex = 0;
	for( uint8_t blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
	{
		for( uint8_t blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			for( uint8_t blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
			{
				if( chunk->blocks[blockX][blockY][blockZ] == BT_AIR ) {
					continue;
				}

				// get neighbouring blocks
				BLOCK_TYPE blockPosY = blockY == CHUNK_HEIGHT-1	? BT_AIR											: chunk->blocks[blockX][blockY+1][blockZ];
				BLOCK_TYPE blockNegY = blockY == 0				? BT_DIRT											: chunk->blocks[blockX][blockY-1][blockZ];
				BLOCK_TYPE blockPosX = blockX == CHUNK_WIDTH-1	? chunkPosX->blocks[0][blockY][blockZ]				: chunk->blocks[blockX+1][blockY][blockZ];
				BLOCK_TYPE blockNegX = blockX == 0				? chunkNegX->blocks[CHUNK_WIDTH-1][blockY][blockZ]	: chunk->blocks[blockX-1][blockY][blockZ];
				BLOCK_TYPE blockPosZ = blockZ == CHUNK_WIDTH-1	? chunkPosZ->blocks[blockX][blockY][0]				: chunk->blocks[blockX][blockY][blockZ+1];
				BLOCK_TYPE blockNegZ = blockZ == 0				? chunkNegZ->blocks[blockX][blockY][CHUNK_WIDTH-1]	: chunk->blocks[blockX][blockY][blockZ-1];

				BLOCK_TYPE blockPosXPosZ;
				if( blockX == CHUNK_WIDTH-1 && blockZ == CHUNK_WIDTH-1 )
				{
					blockPosXPosZ = chunkPosXPosZ->blocks[0][blockY][0];
				}
				else
				{
					if( blockX == CHUNK_WIDTH-1 ) {
						blockPosXPosZ = chunkPosX->blocks[0][blockY][blockZ+1];
					}
					else if( blockZ == CHUNK_WIDTH-1 ) {
						blockPosXPosZ = chunkPosZ->blocks[blockX+1][blockY][0];
					}
					else {
						blockPosXPosZ = chunkPosZ->blocks[blockX+1][blockY][blockZ+1];
					}
				}

				BLOCK_TYPE blockNegXPosZ;
				if( blockX == 0 && blockZ == CHUNK_WIDTH-1 )
				{
					blockNegXPosZ = chunkNegXPosZ->blocks[CHUNK_WIDTH-1][blockY][0];
				}
				else
				{
					if( blockX == 0 ) {
						blockNegXPosZ = chunkNegX->blocks[CHUNK_WIDTH-1][blockY][blockZ+1];
					}
					else if( blockZ == CHUNK_WIDTH-1 ) {
						blockNegXPosZ = chunkPosZ->blocks[blockX-1][blockY][0];
					}
					else {
						blockNegXPosZ = chunkPosZ->blocks[blockX-1][blockY][blockZ+1];
					}
				}

				BLOCK_TYPE blockPosXNegZ;
				if( blockX == CHUNK_WIDTH-1 && blockZ == 0 )
				{
					blockPosXNegZ = chunkPosXNegZ->blocks[0][blockY][CHUNK_WIDTH-1];
				}
				else
				{
					if( blockX == CHUNK_WIDTH-1 ) {
						blockPosXNegZ = chunkPosX->blocks[0][blockY][blockZ+1];
					}
					else if( blockZ == 0 ) {
						blockPosXNegZ = chunkNegZ->blocks[blockX+1][blockY][CHUNK_WIDTH-1];
					}
					else {
						blockPosXNegZ = chunkPosZ->blocks[blockX+1][blockY][blockZ-1];
					}
				}

				BLOCK_TYPE blockNegXNegZ;
				if( blockX == 0 && blockZ == 0 )
				{
					blockNegXNegZ = chunkNegXNegZ->blocks[CHUNK_WIDTH-1][blockY][CHUNK_WIDTH-1];
				}
				else
				{
					if( blockX == 0 ) {
						blockNegXNegZ = chunkNegX->blocks[CHUNK_WIDTH-1][blockY][blockZ-1];
					}
					else if( blockZ == 0 ) {
						blockNegXNegZ = chunkNegZ->blocks[blockX-1][blockY][CHUNK_WIDTH-1];
					}
					else {
						blockNegXNegZ = chunkPosZ->blocks[blockX-1][blockY][blockZ-1];
					}
				}

				if( blockPosX == BT_AIR )
				{
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_X );
					vertexIndex += VERTS_PER_FACE;
				}
				if( blockNegX == BT_AIR )
				{
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_X );
					vertexIndex += VERTS_PER_FACE;
				}
				if( blockPosZ == BT_AIR )
				{
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Z );
					vertexIndex += VERTS_PER_FACE;
				}
				if( blockNegZ == BT_AIR )
				{
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_Z );
					vertexIndex += VERTS_PER_FACE;
				}
				if( blockPosY == BT_AIR )
				{
					AddFace( chunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Y );
					vertexIndex += VERTS_PER_FACE;
				}
				if( blockNegY == BT_AIR )
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