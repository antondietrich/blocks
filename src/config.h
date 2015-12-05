#ifndef __BLOCKS_CONFIG__
#define __BLOCKS_CONFIG__

namespace Blocks
{

struct ConfigType {
	uint fullscreen;
	uint vsync;
	uint screenWidth;
	uint screenHeight;
	uint multisampling;
	uint filtering;
	uint viewDistanceChunks;
};
extern ConfigType Config;

}

#endif
