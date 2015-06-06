#ifndef __BLCOKS_WORLD__
#define __BLCOKS_WORLD__

#include <cstdlib>

namespace Blocks
{

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
#define BLOCK_SIZE 1.0f

enum BLOCK_TYPE
{
	BT_AIR,
	BT_DIRT,
	BT_GRASS
};

struct Chunk
{
	BLOCK_TYPE blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];
};

struct World
{
	Chunk chunks[32][32];
};
	
void GenerateWorld( World *world );
}

#endif // __BLCOKS_WORLD__