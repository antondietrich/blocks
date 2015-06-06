#include "world.h"

namespace Blocks
{

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
						int height = (rand() / (float)RAND_MAX) * 3.0f;
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

}