/**************************************
*
* Block vertex format:
* 	property		bits
* 	position.x		8
* 	position.y		8
* 	position.z		8
* 	normalID		3
* 	texcoordID		2
* 	occluded		1
* 	unused			2
* 					--
* 					32
/**************************************/


#ifndef __BLCOKS_WORLD__
#define __BLCOKS_WORLD__

#include <cstdlib>
#include <string>
#include <stdint.h>

namespace Blocks
{

#define CHUNKS_TO_GENERATE 12
#define HALF_CHUNKS_TO_GENERATE (CHUNKS_TO_GENERATE / 2)

#define CHUNK_WIDTH 32
#define CHUNK_HEIGHT 255
#define BLOCK_SIZE 1.0f

#define FACES_PER_BLOCK 6
#define VERTS_PER_FACE 6

#define CHUNKS_TO_DRAW 2
// TODO: it's not a radius, rename
#define VISIBLE_CHUNKS_RADIUS (CHUNKS_TO_DRAW * 2 + 1)

#define MAX_VERTS_PER_CHUNK_MESH (CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT * FACES_PER_BLOCK * VERTS_PER_FACE)
	//9437184

enum BLOCK_TYPE
{
	BT_AIR,
	BT_DIRT,
	BT_GRASS
};

struct BlockVertex
{
	uint8_t data[4];
};

struct Chunk
{
	BLOCK_TYPE blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];
};

struct ChunkMesh
{
	int chunkPos[2];
	int size;
	BlockVertex* vertices;
};

struct World
{
	Chunk chunks[CHUNKS_TO_GENERATE][CHUNKS_TO_GENERATE];
};
	
void InitWorldGen();
void GenerateWorld( World *world );
int GenerateChunkMesh( ChunkMesh *chunkMesh, Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
											 Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
											 Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ );

int MeshCacheIndexFromChunkPos( unsigned int x, unsigned int z );

}

#endif // __BLCOKS_WORLD__