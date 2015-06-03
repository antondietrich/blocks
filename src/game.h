#ifndef __BLOCKS_GAME__
#define __BLOCKS_GAME__

#include <assert.h>
#include <DirectXMath.h>

#include "renderer.h"
#include "world.h"
#include "config.h"

namespace Blocks
{

#define VK_LMB 0xfe
#define VK_MMB 0xfd
#define VK_RMB 0xfc
#define VK_NUMPAD_ENTER 0xff
#define NUM_VKEYS 256

struct UserInput
{
	bool key[ NUM_VKEYS ];
	struct {
		long x;
		long y;
	} mouse;
};

#define UPDATE_DELTA_FRAMES 30

class Game
{
public:
	Game();
	~Game();
	bool Start( HWND wnd );
	void DoFrame( float dt );

	UserInput input;
	Renderer renderer;
	Overlay overlay;
private:
	static bool isInstantiated_;

	World *world_;

};
	
}

#endif // __BLOCKS_GAME__