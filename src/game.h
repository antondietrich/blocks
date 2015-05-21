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

	Renderer renderer;
private:
	static bool isInstantiated_;

};
	
}

#endif // __BLOCKS_GAME__