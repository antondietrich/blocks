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
};
extern ConfigType Config;

}

#endif