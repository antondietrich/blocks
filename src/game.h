#ifndef __BLOCKS_GAME__
#define __BLOCKS_GAME__

#include <assert.h>
#include <DirectXMath.h>

#include "renderer.h"
#include "world.h"
#include "config.h"

namespace Blocks
{

#define UPDATE_DELTA_FRAMES 30

class Game
{
public:
	Game();
	~Game();
	bool Start( HWND wnd );
	void DoFrame( float dt );

	Renderer renderer;
	Overlay overlay;
private:
	static bool isInstantiated_;

	World *world_;

};
	
}

#endif // __BLOCKS_GAME__