#ifndef __BLOCKS_CONFIG__
#define __BLOCKS_CONFIG__

namespace Blocks
{

struct ConfigType {
	bool fullscreen;
	int screenWidth;
	int screenHeight;
	int multisampling;
};
extern ConfigType Config;

}

#endif