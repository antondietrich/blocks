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

#define CHUNKS_TO_DRAW 7
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
	uint8_t pos[4];
	uint8_t normal[4];
	uint8_t texcoord[4];
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
	
void GenerateWorld( World *world );
int MeshCacheIndexFromChunkPos( unsigned int x, unsigned int z );

enum FACE_INDEX
{
	FACE_NEG_Z = 0,
	FACE_POS_X = 6,
	FACE_POS_Z = 12,
	FACE_NEG_X = 18,
	FACE_POS_Y = 24,
	FACE_NEG_Y = 30
};

void AddFace( BlockVertex *vertexBuffer, int vertexIndex, int blockX, int blockY, int blockZ, FACE_INDEX faceIndex );

}

#endif // __BLCOKS_WORLD__