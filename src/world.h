/**************************************
*
* Block vertex format:
* 	property		bits
* 	position.x		8
* 	position.y		8
* 	position.z		8
* 	normalID		3
* 	texcoordID		5
* 					--
* 					32
*
* 	unused			30
* 	occlusion		2
*					--
*					32
/**************************************/


#ifndef __BLCOKS_WORLD__
#define __BLCOKS_WORLD__

#include <cstdlib>
#include <string>
#include <DirectXMath.h>
//#include <stdint.h>
#include "types.h"

namespace Blocks
{

//#define VIEW_DISTANCE 6
//#define MESH_CACHE_DIM (VIEW_DISTANCE * 2 + 1)
//
//#define CHUNK_CACHE_HALF_DIM (VIEW_DISTANCE + 2)
//#define CHUNK_CACHE_DIM (CHUNK_CACHE_HALF_DIM * 2 + 1)

#define CHUNK_WIDTH 32
#define CHUNK_HEIGHT 255
#define BLOCK_SIZE 1.0f

#define FACES_PER_BLOCK 6
#define VERTS_PER_FACE 6


#define MAX_VERTS_PER_CHUNK_MESH (CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT * FACES_PER_BLOCK * VERTS_PER_FACE)

enum BLOCK_TYPE
{
	BT_AIR,
	BT_DIRT,
	BT_GRASS,
	BT_STONE,
	BT_WOOD
};

struct BlockPosition
{
	int chunkX;
	int chunkZ;
	int x, y, z;
};

struct BlockVertex
{
	uint8 data[4];
	uint32 texID;
};

struct Chunk
{
	int pos[2];
	BLOCK_TYPE blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];
};

struct ChunkMesh
{
	int chunkPos[2];
	int size;
	bool dirty;
	BlockVertex* vertices;
};

//struct World
//{
//	Chunk chunks[CHUNK_CACHE_DIM * CHUNK_CACHE_DIM];
//};

void InitWorldGen();
//void GenerateWorld( World *world );
void PrefillChunkCache( Chunk * cache, uint cacheDim );
void GenerateChunk( Chunk *chunk, int x, int z );
int GenerateChunkMesh( ChunkMesh *chunkMesh, Chunk* chunkNegXPosZ, Chunk* chunkPosZ, Chunk* chunkPosXPosZ,
											 Chunk* chunkNegX, Chunk* chunk, Chunk* chunkPosX,
											 Chunk* chunkNegXNegZ, Chunk* chunkNegZ, Chunk* chunkPosXNegZ );

DirectX::XMINT3 GetPlayerChunkPos( DirectX::XMFLOAT3 playerPos );
DirectX::XMINT3 GetPlayerBlockPos( DirectX::XMFLOAT3 playerPos );
BLOCK_TYPE GetBlockType( const Chunk &chunk, DirectX::XMINT3 blockPos );
BLOCK_TYPE SetBlockType( Chunk *chunk, DirectX::XMINT3 blockPos, BLOCK_TYPE type );

int ChunkCacheIndexFromChunkPos( uint x, uint z, uint cacheDim );
int MeshCacheIndexFromChunkPos( uint x, uint z, uint cacheDim );


}

#endif // __BLCOKS_WORLD__
