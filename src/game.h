#ifndef __BLOCKS_GAME__
#define __BLOCKS_GAME__

#include <assert.h>

#include "renderer.h"
#include "config.h"

namespace Blocks
{

class Game
{
public:
	Game();
	~Game();
	bool Start( HWND wnd );
	void DoFrame();
private:
	static bool isInstantiated_;

	Renderer renderer_;
};
	
}

#endif