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

	if( !overlay.Start( &renderer ) )
	{
		return false;
	}


	return true;
}

void Game::DoFrame()
{
	renderer.Begin();
	renderer.Present();
	overlay.Print( "Hello, world!" );
	
	renderer.End();
}

}