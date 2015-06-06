#ifndef __BLOCKS_CONFIG__
#define __BLOCKS_CONFIG__

namespace Blocks
{

struct ConfigType {
	bool fullscreen;
	bool vsync;
	int screenWidth;
	int screenHeight;
	int multisampling;
	unsigned int filtering;
};
extern ConfigType Config;

}

#endif