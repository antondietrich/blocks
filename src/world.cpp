#define BLOCK_DEF_IMPLEMENTATION
#include "world.h"


namespace Blocks
{

using namespace DirectX;

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

// Return a random value in the range [min, max)
// TODO: rand() may cause data races in threaded code.
//  Need a different RNG in the future
int Random( int min, int max )
{
	return (rand() % ( max - min ) ) + min;
}

float Cubic( float pL, float p0, float p3, float pR, float t )
{
	float p1 = p0 + ( p3 - pL ) / 6;
	float p2 = p3 - ( pR - p0 ) / 6;

	float A = (1-t) * (1-t) * (1-t);
	float B = 3 * t * (1-t) * (1-t);
	float C = 3 * t * t * (1-t);
	float D = t * t * t;

	float result = A*p0 + B*p1 + C*p2 + D*p3;
	return result;
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

BlockVertex *gChunkVertexBuffer;

void InitWorldGen()
{
	srand( 122345 );
	gChunkVertexBuffer = new BlockVertex[ MAX_VERTS_PER_CHUNK_MESH ];
}

struct ChunkHeightField
{
	int height[CHUNK_WIDTH][CHUNK_WIDTH];
};

float PerlinLinear( int x, int y, int scale )
{
	int x0 = (int)floor( (float)x / scale ) * scale;
	int y0 = (int)floor( (float)y / scale ) * scale;
	int x1 = x0 + scale;
	int y1 = y0 + scale;

	float r0 = Noise2D( x0, y0 );
	float r1 = Noise2D( x1, y0 );
	float r2 = Noise2D( x0, y1 );
	float r3 = Noise2D( x1, y1 );

	float tX = (float)( x - x0 ) / scale;
	float tY = (float)( y - y0 ) / scale;

	float l0 = Lerp( r0, r1, tX );
	float l1 = Lerp( r2, r3, tX );
	float p = Lerp( l0, l1, tY );

	return p;
}

float PerlinCubic( int x, int y, int scale )
{
	// closest anchor points
	int x1 = (int)floor( (float)x / scale ) * scale;
	int y1 = (int)floor( (float)y / scale ) * scale;
	int x2 = x1 + scale;
	int y2 = y1 + scale;

	// next anchor points for computing tangents
	int x0 = x1 - scale;
	int y0 = y1 - scale;
	int x3 = x2 + scale;
	int y3 = y2 + scale;

	// compute 4x4 heights at anchor points, rXY
	float r00 = Noise2D( x0, y0 );
	float r10 = Noise2D( x1, y0 );
	float r20 = Noise2D( x2, y0 );
	float r30 = Noise2D( x3, y0 );

	float r01 = Noise2D( x0, y1 );
	float r11 = Noise2D( x1, y1 );
	float r21 = Noise2D( x2, y1 );
	float r31 = Noise2D( x3, y1 );

	float r02 = Noise2D( x0, y2 );
	float r12 = Noise2D( x1, y2 );
	float r22 = Noise2D( x2, y2 );
	float r32 = Noise2D( x3, y2 );

	float r03 = Noise2D( x0, y3 );
	float r13 = Noise2D( x1, y3 );
	float r23 = Noise2D( x2, y3 );
	float r33 = Noise2D( x3, y3 );

	// interpolation parameters
	float tX = (float)( x - x1 ) / scale;
	float tY = (float)( y - y1 ) / scale;

	// interpolate along x
	float c0 = Cubic( r00, r10, r20, r30, tX );
	float c1 = Cubic( r01, r11, r21, r31, tX );
	float c2 = Cubic( r02, r12, r22, r32, tX );
	float c3 = Cubic( r03, r13, r23, r33, tX );

	float p = Cubic( c0, c1, c2, c3, tY );
	return p;
}

#if 1

void GenerateChunk( Chunk *chunk, int x, int z )
{
	chunk->pos[0] = x;
	chunk->pos[1] = z;
	chunk->terrainGenerated = true;

	ZeroMemory( chunk->blocks, sizeof( BLOCK_TYPE ) * CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT );

	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			float p0 = PerlinCubic( x * CHUNK_WIDTH + blockX, z * CHUNK_WIDTH + blockZ, (int)(0.5f * CHUNK_WIDTH) );
			float p1 = PerlinCubic( x * CHUNK_WIDTH + blockX, z * CHUNK_WIDTH + blockZ, 2 * CHUNK_WIDTH );
			float pbiome = PerlinCubic( x * CHUNK_WIDTH + blockX, z * CHUNK_WIDTH + blockZ, 8 * CHUNK_WIDTH );

			BLOCK_TYPE biomeBlockType;
			if( pbiome > 0.4f )
			{
				chunk->biomeMap[ blockX ][ blockZ ] = BIOME_PLAINS;
				biomeBlockType = BT_GRASS;
			}
			else
			{
				chunk->biomeMap[ blockX ][ blockZ ] = BIOME_MOUNTAINS;
				biomeBlockType = BT_STONE;
			}

			float mountainsScale1 = pbiome > 0.4f ? 0.0f : ( 0.4f - pbiome ) * 2.5f;
			mountainsScale1 = sqrt( mountainsScale1 );
			float mountainsScale2 = pbiome > 0.4f ? 0.2f : 0.2f + ( 0.4f - pbiome ) * 2.5f;

			//int height = (int)( p1 * 30 );
			int height = (int)(p0 * mountainsScale1 * 10) + (int)( p1 * 60 * mountainsScale2 );

			for( int blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
			{
				if( chunk->blocks[blockX][blockY][blockZ] != 0 )
					continue;

				if( blockY < height ) {
					chunk->blocks[blockX][blockY][blockZ] = BT_DIRT;
				}
				else if( blockY == height ) {
					chunk->blocks[blockX][blockY][blockZ] = biomeBlockType;
				}
				else {
					chunk->blocks[blockX][blockY][blockZ] = BT_AIR;
				}
			}
		}
	} // end generate landscape
}

void GenerateChunkStructures( Chunk * chunk, ChunkContext chunkContext )
{
	for( int offsetZ = -1; offsetZ <= 1; offsetZ++ )
	{
		for( int offsetX = -1; offsetX <= 1; offsetX++ )
		{
			if( offsetZ == 0 && offsetX == 0 )
				continue;

			int baseX = 1, baseZ = 1;
			// ClearChunk( chunkContext.chunks_[ (baseZ + offsetZ)*3 + (baseX + offsetX) ] );
			int index = (baseZ + offsetZ)*3 + (baseX + offsetX);
			Chunk * meta = chunkContext.chunks_[ index ];

			assert( "Invalid meta chunk!" && ( meta->pos[0] == chunk->pos[0] + offsetX || meta->pos[0] == INT_MAX ) );
			assert( "Invalid meta chunk!" && ( meta->pos[1] == chunk->pos[1] + offsetZ || meta->pos[1] == INT_MAX ) );

			#if 0
			if( meta->pos[0] == INT_MAX && meta->pos[1] == INT_MAX )
			{
				for( int x = 0; x < CHUNK_WIDTH; x++ )
				{
					for( int z = 0; z < CHUNK_WIDTH; z++ )
					{
						for( int y = 0; y < CHUNK_HEIGHT; y++ )
						{
							if( meta->blocks[x][y][z] != BT_AIR )
								assert( !"Non empty meta with invalid coords" );
						}
					}
				}
			}
			#endif

			meta->terrainGenerated = true;
			meta->pos[0] = chunk->pos[0] + offsetX;
			meta->pos[1] = chunk->pos[1] + offsetZ;
		}
	}

	// generate trees
	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			if( chunk->biomeMap[blockX][blockZ] == BIOME_PLAINS )
			{
				if( ( blockX % 16 == 0 ) &&
					( blockZ % 16 == 0 ) )
				{

					int offsetX = Random( 0, 5 ) - 2;
					int offsetZ = Random( 0, 5 ) - 2;

					offsetX = 0;
					offsetZ = 0;

					if( !InRange( blockX + offsetX, 0, CHUNK_WIDTH ) ||
						!InRange( blockZ + offsetZ, 0, CHUNK_WIDTH ) )
						continue;

					int groundHeight = CHUNK_HEIGHT - 1;
					for( ; groundHeight >= 0; groundHeight-- )
					{
						if( chunk->blocks[ blockX + offsetX ][ groundHeight ][ blockZ + offsetZ ] != BT_AIR )
							break;
					}

					if( chunk->blocks[ blockX + offsetX ][ groundHeight ][ blockZ + offsetZ ] != BT_GRASS )
						continue;

					// int treeHeight = Random( 8, 14 );
					int treeHeight = 11;
					int treeHeightOffset = (int)floor( Noise2D( blockX, blockZ ) * 4.0f - 4.0f);
					treeHeight += treeHeightOffset;

					for( int height = 1; height <= treeHeight; height++ )
					{
						chunk->blocks[ blockX + offsetX ][ groundHeight + height ][ blockZ + offsetZ ] = BT_TREE_TRUNK;

						if( height == treeHeight )
							chunk->blocks[ blockX + offsetX ][ groundHeight + height ][ blockZ + offsetZ ] = BT_TREE_LEAVES;

						if( height >= treeHeight - 4 )
						{
							int leafRadius = 0;

							if( height == treeHeight - 4 )
								leafRadius = 3;
							else if( height == treeHeight - 3 ||
									 height == treeHeight - 2 )
								leafRadius = 2;
							else
								leafRadius = 1;

							for( int leafX = -leafRadius;
									 leafX <= leafRadius;
									 leafX++ )
							{
								for( int leafZ = -leafRadius;
										 leafZ <= leafRadius;
										 leafZ++ )
								{
									if( leafX == 0 && leafZ == 0 )
										continue;

									// remove corners
									if( ( leafX == -leafRadius && leafZ == -leafRadius ) ||
										( leafX ==  leafRadius && leafZ == -leafRadius ) ||
										( leafX == -leafRadius && leafZ ==  leafRadius ) ||
										( leafX ==  leafRadius && leafZ ==  leafRadius ) )
										continue;

									#if 0
									if( !InRange( blockX + offsetX + leafX, 0, CHUNK_WIDTH ) ||
										!InRange( blockZ + offsetZ + leafZ, 0, CHUNK_WIDTH ) )
										continue;
									#endif

									if( chunkContext.GetBlockAt( blockX + offsetX + leafX, groundHeight + height, blockZ + offsetZ + leafZ ) == BT_AIR )
									{
										// chunk->blocks[blockX + offsetX + leafX][groundHeight + height][blockZ + offsetZ + leafZ] = BT_TREE_LEAVES;
										chunkContext.SetBlockAt( blockX + offsetX + leafX, groundHeight + height, blockZ + offsetZ + leafZ, BT_TREE_LEAVES );
									}
								}
							}
						}
					}
				}
			}
		}
	} // end generate trees
}

void MergeChunks( Chunk * target, Chunk * source )
{
	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			for( int blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
			{
				if( target->blocks[blockX][blockY][blockZ] == BT_AIR &&
					source->blocks[blockX][blockY][blockZ] != BT_AIR )
				{
					target->blocks[blockX][blockY][blockZ] = source->blocks[blockX][blockY][blockZ];
				}
			}
		}
	}
}

void ClearChunk( Chunk * chunk )
{
	memset( chunk->blocks, BT_AIR, CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT * sizeof( BLOCK_TYPE ) );
	//chunk->terrainGenerated = false;
}

#else

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
	ChunkHeightField heightfield2 = {};

	GenerateChunkHeightField( &heightfield1, chunk, x, z, 8, 10 );
	GenerateChunkHeightField( &heightfield2, chunk, x, z, 2, 60 );
	// GenerateChunkHeightField( &heightfield2, chunk, x, z, 8, 20 );

	ChunkHeightField biomeField = {};
	GenerateChunkHeightField( &biomeField, chunk, x, z, 8, 100 );

	for( int blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
	{
		for( int blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			BLOCK_TYPE biomeBlockType;
			if( biomeField.height[blockZ][blockX] / 100.0f > 0.4f )
			{
				biomeBlockType = BT_GRASS;
			}
			else
			{
				biomeBlockType = BT_STONE;
			}

			float mountainsScale = biomeField.height[blockZ][blockX] / 100.0f > 0.4f ? 0.2f : 0.2f + ( 0.4f - biomeField.height[blockZ][blockX] / 100.0f ) * 2.5f;


			int height = heightfield1.height[blockZ][blockX] + (int)( heightfield2.height[blockZ][blockX] * mountainsScale );

			for( int blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
			{
				if( blockY < height ) {
					chunk->blocks[blockX][blockY][blockZ] = BT_DIRT;
				}
				else if( blockY == height ) {
					chunk->blocks[blockX][blockY][blockZ] = biomeBlockType;
				}
				else {
					chunk->blocks[blockX][blockY][blockZ] = BT_AIR;
				}
			}
		}
	}
}
#endif

void InitChunkCache( Chunk * cache, uint cacheDim )
{
	for( uint i = 0; i < cacheDim * cacheDim; i++ )
	{
		ClearChunk( &cache[i] );
		cache[i].pos[0] = INT_MAX;
		cache[i].pos[1] = INT_MAX;
	}
}

XMINT3 GetPlayerChunkPos( XMFLOAT3 playerPos )
{
	XMINT3 chunkPos = {};

	if( playerPos.x >= 0 ) {
		chunkPos.x = (int)playerPos.x / CHUNK_WIDTH;
	}
	else {
		chunkPos.x = (int)playerPos.x / CHUNK_WIDTH - 1;
	}

	if( playerPos.z >= 0 ) {
		chunkPos.z = (int)playerPos.z / CHUNK_WIDTH;
	}
	else {
		chunkPos.z = (int)playerPos.z / CHUNK_WIDTH - 1;
	}

	return chunkPos;
}

XMINT3 GetPlayerBlockPos( XMFLOAT3 playerPos )
{
	XMINT3 blockPos = {};
	XMINT3 chunkPos = GetPlayerChunkPos( playerPos );

	blockPos.x = (int)floor( playerPos.x - chunkPos.x * CHUNK_WIDTH );
	blockPos.z = (int)floor( playerPos.z - chunkPos.z * CHUNK_WIDTH );
	blockPos.y = (int)floor( playerPos.y );

	return blockPos;
}

BLOCK_TYPE GetBlockType( const Chunk &chunk, XMINT3 blockPos )
{
	return chunk.blocks[blockPos.x][blockPos.y][blockPos.z];
}

BLOCK_TYPE SetBlockType( Chunk *chunk, DirectX::XMINT3 blockPos, BLOCK_TYPE type )
{
	BLOCK_TYPE prevBlockType = chunk->blocks[blockPos.x][blockPos.y][blockPos.z];
	chunk->blocks[blockPos.x][blockPos.y][blockPos.z] = type;

	return prevBlockType;
}

int ChunkCacheIndexFromChunkPos( uint x, uint z, uint cacheDim )
{
	unsigned int ux = x + INT_MAX;
	unsigned int uz = z + INT_MAX;
	int cacheX = ux % cacheDim;
	int cacheZ = uz % cacheDim;
	int cacheIndex = cacheZ * cacheDim + cacheX;
	return cacheIndex;
}

int MeshCacheIndexFromChunkPos( uint x, uint z, uint cacheDim )
{
	unsigned int ux = x + INT_MAX;
	unsigned int uz = z + INT_MAX;
	int cacheX = ux % cacheDim;
	int cacheZ = uz % cacheDim;
	int cacheIndex = cacheZ * cacheDim + cacheX;
	return cacheIndex;
}

enum BLOCK_OFFSET_DIRECTION
{
	BOD_SAM,
	BOD_POS,
	BOD_NEG,

	BOD_COUNT
};

ChunkContext::ChunkContext( Chunk * chunkNegZNegX, Chunk * chunkNegZSamX, Chunk * chunkNegZPosX,
							Chunk * chunkSamZNegX, Chunk * chunkSamZSamX, Chunk * chunkSamZPosX,
							Chunk * chunkPosZNegX, Chunk * chunkPosZSamX, Chunk * chunkPosZPosX )
{
	chunks_[0] = chunkNegZNegX;
	chunks_[1] = chunkNegZSamX;
	chunks_[2] = chunkNegZPosX;
	chunks_[3] = chunkSamZNegX;
	chunks_[4] = chunkSamZSamX;
	chunks_[5] = chunkSamZPosX;
	chunks_[6] = chunkPosZNegX;
	chunks_[7] = chunkPosZSamX;
	chunks_[8] = chunkPosZPosX;
}

BLOCK_TYPE ChunkContext::GetBlockAt( int x, int y, int z ) const
{
	assert( "Block index out of bounds" && x >= (0 - CHUNK_WIDTH) && x <= (CHUNK_WIDTH + CHUNK_WIDTH - 1) );
	assert( "Block index out of bounds" && z >= (0 - CHUNK_WIDTH) && z <= (CHUNK_WIDTH + CHUNK_WIDTH - 1) );

	Chunk * chunkToAccess = 0;
	int offsetX = 1, offsetZ = 1;
	if( x < 0 )
	{
		offsetX -= 1;
		x = CHUNK_WIDTH + x;
	}
	if( x >= CHUNK_WIDTH )
	{
		offsetX += 1;
		x = x - CHUNK_WIDTH;
	}
	if( z < 0 )
	{
		offsetZ -= 1;
		z = CHUNK_WIDTH + z;
	}
	if( z >= CHUNK_WIDTH )
	{
		offsetZ += 1;
		z = z - CHUNK_WIDTH;
	}

	chunkToAccess = chunks_[ 3*offsetZ + offsetX ];

	return chunkToAccess->blocks[x][y][z];
}

void ChunkContext::SetBlockAt( int x, int y, int z, BLOCK_TYPE block )
{
	assert( "Block index out of bounds" && x >= (0 - CHUNK_WIDTH) && x <= (CHUNK_WIDTH + CHUNK_WIDTH - 1) );
	assert( "Block index out of bounds" && z >= (0 - CHUNK_WIDTH) && z <= (CHUNK_WIDTH + CHUNK_WIDTH - 1) );

	Chunk * chunkToAccess = 0;
	int offsetX = 1, offsetZ = 1;
	if( x < 0 )
	{
		offsetX -= 1;
		x = CHUNK_WIDTH + x;
	}
	if( x >= CHUNK_WIDTH )
	{
		offsetX += 1;
		x = x - CHUNK_WIDTH;
	}
	if( z < 0 )
	{
		offsetZ -= 1;
		z = CHUNK_WIDTH + z;
	}
	if( z >= CHUNK_WIDTH )
	{
		offsetZ += 1;
		z = z - CHUNK_WIDTH;
	}

	chunkToAccess = chunks_[ 3*offsetZ + offsetX ];

	chunkToAccess->blocks[x][y][z] = block;
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

#define PACK_NORMAL_AND_TEXCOORD( normalIndex, texcoordIndex ) (((normalIndex) << 5) | (texcoordIndex << 3) | 0)
#define PACK_OCCLUSION_AND_TEXID( occlusion, texID ) (uint8)(( (occlusion) << 6 ) | (texID) | 0)
#define SET_TEXCOORD( texcoordIndex ) ( (texcoordIndex << 3) & 0x1F );
#define SET_NORMAL( normalIndex ) ( (normalIndex) << 5) & 0xE0 );

BlockVertex standardBlock[] =
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

BlockVertex * blockMeshes[] = { standardBlock };

void AddFace( BlockVertex *vertexBuffer, int startVertexIndex,
			  uint8 blockX, uint8 blockY, uint8 blockZ,
			  FACE_INDEX faceIndex, uint8 occluded[4], BlockDefinition blockDefinition )
{
	uint textureSide = BFD_SIDE;
	if( faceIndex == FACE_POS_Y ||
		faceIndex == FACE_NEG_Y )
	{
		textureSide = BFD_TOP;
	}
	vertexBuffer[ startVertexIndex + 0 ] = blockMeshes[ blockDefinition.meshID ][ faceIndex + 0];
		vertexBuffer[ startVertexIndex + 0 ].data1[0] += blockX;
		vertexBuffer[ startVertexIndex + 0 ].data1[1] += blockY;
		vertexBuffer[ startVertexIndex + 0 ].data1[2] += blockZ;
		// vertexBuffer[ startVertexIndex + 0 ].data[3] |= SET_TEXCOORD( 0 + blockDefinition.textureCoordsArrayOffset );
		vertexBuffer[ startVertexIndex + 0 ].data2[0] = PACK_OCCLUSION_AND_TEXID( occluded[0], blockDefinition.textureID[ textureSide ] );

	vertexBuffer[ startVertexIndex + 1 ] = blockMeshes[ blockDefinition.meshID ][ faceIndex + 1];
		vertexBuffer[ startVertexIndex + 1 ].data1[0] += blockX;
		vertexBuffer[ startVertexIndex + 1 ].data1[1] += blockY;
		vertexBuffer[ startVertexIndex + 1 ].data1[2] += blockZ;
		// vertexBuffer[ startVertexIndex + 1 ].data[3] |= SET_TEXCOORD( 1 + blockDefinition.textureCoordsArrayOffset );
		vertexBuffer[ startVertexIndex + 1 ].data2[0] = PACK_OCCLUSION_AND_TEXID( occluded[1], blockDefinition.textureID[ textureSide ] );

	vertexBuffer[ startVertexIndex + 4 ] = blockMeshes[ blockDefinition.meshID ][ faceIndex + 2];
		vertexBuffer[ startVertexIndex + 4 ].data1[0] += blockX;
		vertexBuffer[ startVertexIndex + 4 ].data1[1] += blockY;
		vertexBuffer[ startVertexIndex + 4 ].data1[2] += blockZ;
		// vertexBuffer[ startVertexIndex + 4 ].data[3] |= SET_TEXCOORD( 3 + blockDefinition.textureCoordsArrayOffset );
		vertexBuffer[ startVertexIndex + 4 ].data2[0] = PACK_OCCLUSION_AND_TEXID( occluded[2], blockDefinition.textureID[ textureSide ] );

	vertexBuffer[ startVertexIndex + 5 ] = blockMeshes[ blockDefinition.meshID ][ faceIndex + 3];
		vertexBuffer[ startVertexIndex + 5 ].data1[0] += blockX;
		vertexBuffer[ startVertexIndex + 5 ].data1[1] += blockY;
		vertexBuffer[ startVertexIndex + 5 ].data1[2] += blockZ;
		// vertexBuffer[ startVertexIndex + 5 ].data[3] |= SET_TEXCOORD( 2 + blockDefinition.textureCoordsArrayOffset );
		vertexBuffer[ startVertexIndex + 5 ].data2[0] = PACK_OCCLUSION_AND_TEXID( occluded[3], blockDefinition.textureID[ textureSide ] );

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

int GenerateChunkMesh( ChunkMesh *chunkMesh, ChunkContext chunks )
{
	int vertexIndex = 0;
	for( uint8 blockY = 0; blockY < CHUNK_HEIGHT; blockY++ )
	{
		for( uint8 blockX = 0; blockX < CHUNK_WIDTH; blockX++ )
		{
			for( uint8 blockZ = 0; blockZ < CHUNK_WIDTH; blockZ++ )
			{
				if( chunks.GetBlockAt( blockX, blockY, blockZ ) == BT_AIR ) {
					continue;
				}

				BlockDefinition blockDefinition = blockDefinitions[ chunks.GetBlockAt( blockX, blockY, blockZ ) ];

				uint8 occlusion[] = { 0, 0, 0, 0 };

				// X+
				if( blockDefinitions[ chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ + 0 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ - 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ - 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_X, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}

				// X-
				if( blockDefinitions[ chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ + 0 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ - 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ - 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_X, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}

				// Z+
				if( blockDefinitions[ chunks.GetBlockAt( blockX + 0, blockY + 0, blockZ + 1 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Z, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}

				// Z-
				if( blockDefinitions[ chunks.GetBlockAt( blockX + 0, blockY + 0, blockZ - 1 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ - 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ - 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ - 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 0, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ - 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_Z, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}

				// Y+
				if( blockDefinitions[ chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ + 0 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY + 1, blockZ - 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ - 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY + 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY + 1, blockZ + 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_POS_Y, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}

				// Y-
				if( blockDefinitions[ chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ + 0 ) ].transparent )
				{
					occlusion[0] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ - 1 ) );
					occlusion[1] = VertexAO( chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX - 1, blockY - 1, blockZ + 1 ) );
					occlusion[2] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ + 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 1 ) );
					occlusion[3] = VertexAO( chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ + 0 ),
											 chunks.GetBlockAt( blockX + 0, blockY - 1, blockZ - 1 ),
											 chunks.GetBlockAt( blockX + 1, blockY - 1, blockZ - 1 ) );
					AddFace( gChunkVertexBuffer, vertexIndex, blockX, blockY, blockZ, FACE_NEG_Y, occlusion, blockDefinition );
					vertexIndex += VERTS_PER_FACE;
				}
			}
		}
	}

	chunkMesh->vertices = new BlockVertex[ vertexIndex ];
	memcpy( chunkMesh->vertices, gChunkVertexBuffer, sizeof( BlockVertex ) * vertexIndex );
	chunkMesh->numVertices = vertexIndex;

	return vertexIndex;
}

}
