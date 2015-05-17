#include "game.h"

using namespace Blocks;

bool Game::isInstantiated_ = false;

Game::Game()
:
renderer_()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
}

Game::~Game()
{

}

bool Game::Start( HWND wnd )
{
	if( !renderer_.Start( wnd ) )
	{
		return false;
	}

	return true;
}

void Game::DoFrame()
{
	renderer_.Present();
}