#include "world.h"

namespace Blocks
{

void GenerateWorld( World *world )
{
	for( int z = 0; z < 32; z++ )
	{
		for( int x = 0; x < 32; x++ )
		{
			for( int chunkY = 0; chunkY < CHUNK_HEIGHT; chunkY++ )
			{
				for( int chunkZ = 0; chunkZ < CHUNK_WIDTH; chunkZ++ )
				{
					for( int chunkX = 0; chunkX < CHUNK_WIDTH; chunkX++ )
					{
						if( chunkY < 12 ) {
							world->chunks[x][z].blocks[chunkX][chunkY][chunkZ] = BT_DIRT;
						}
						else if( chunkY == 12 ) {
							world->chunks[x][z].blocks[chunkX][chunkY][chunkZ] = BT_GRASS;
						}
						else {
							world->chunks[x][z].blocks[chunkX][chunkY][chunkZ] = BT_AIR;
						}
					}
				}
			}
		}
	}
}

}