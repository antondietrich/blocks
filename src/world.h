#ifndef __BLCOKS_WORLD__
#define __BLCOKS_WORLD__

#include <cstdlib>
#include <string>
#include <stdint.h>

namespace Blocks
{

#define CHUNK_WIDTH 32
#define CHUNK_HEIGHT 256
#define BLOCK_SIZE 1.0f

#define VERTS_PER_FACE 6

#define CHUNKS_TO_DRAW 10
// TODO: it's not a radius, rename
#define VISIBLE_CHUNKS_RADIUS (CHUNKS_TO_DRAW * 2 + 1)

#define MAX_VERTS_PER_CHUNK_MESH (CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT * VERTS_PER_FACE * 6)

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
	Chunk chunks[32][32];
};
	
void InitWorldGen();
void GenerateWorld( World *world );
int GenerateChunkMesh( ChunkMesh *chunkMesh, Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
											 Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
											 Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ );

int MeshCacheIndexFromChunkPos( unsigned int x, unsigned int z );

}

#endif // __BLCOKS_WORLD__