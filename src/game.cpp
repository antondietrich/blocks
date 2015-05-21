#include "game.h"

namespace Blocks
{

bool Game::isInstantiated_ = false;

Game::Game()
:
renderer()
{
	assert( !isInstantiated_ );
	isInstantiated_ = true;
}

Game::~Game()
{

}

bool Game::Start( HWND wnd )
{
	if( !renderer.Start( wnd ) )
	{
		return false;
	}

	return true;
}

void Game::DoFrame()
{
	renderer.Present();
}

}