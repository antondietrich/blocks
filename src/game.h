#ifndef __BLOCKS_GAME__
#define __BLOCKS_GAME__

#include <assert.h>
#include <DirectXMath.h>
#include <limits.h>


#include "lib/win32_file.h"
#include "lib/obj_importer.h"
#include "types.h"
#include "directxmathex.h"
#include "input.h"
#include "renderer.h"
#include "world.h"
#include "collision.h"
#include "config.h"

namespace Blocks
{

#define VK_LMB 0xfe
#define VK_MMB 0xfd
#define VK_RMB 0xfc
#define VK_NUMPAD_ENTER 0xff
#define NUM_VKEYS 256

#define UPDATE_DELTA_FRAMES 30

#define MAX_GAME_OBJECTS 128

struct GameObject
{
	Transform transform;
	Mesh * mesh;
	Material * material;
};

struct GameTime
{
	uint year;
	uint month; // 1 .. 12
	uint day; // 1 .. 31
	uint hours; // 0 .. 23
	uint minutes; // 0 .. 59
	float seconds; // 0.0f..59.(9)f

	// game seconds per real-world second
	float scale;

	void AdvanceTime( float ms );
	void DecreaseTime( float ms );
};

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

	GameTime gameTime_;
};

}

#endif // __BLOCKS_GAME__
