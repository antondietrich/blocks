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
	//renderer.Present();
	overlay.Reset();
	overlay.Print( "Hello, world!" );
	overlay.Print( "Second line,\nwoah!" );
	overlay.Print( "And a very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, long line!!" );

	renderer.End();
}

}