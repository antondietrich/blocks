#include <Windows.h>

#include "game.h"
#include "config.h"
#include "utils.h"

using namespace Blocks;

ConfigType Blocks::Config;

LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam );
bool LoadConfig();
bool RegisterRawInputDevices( HWND hwnd );
void TranslateUserInput( UserInput &userInput, LPARAM lparam );

int __stdcall wWinMain( HINSTANCE thisInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow )
{
	UNREFERENCED_PARAMETER( prevInstance );
	UNREFERENCED_PARAMETER( cmdLine );

	/* Load config */
	if( !LoadConfig() )
	{
		OutputDebugStringA( "Failed to load config!" );
		return 1;
	}

	/* Create a window */
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.hInstance = thisInstance;
	windowClass.hIcon = LoadIcon(thisInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	windowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	windowClass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "BlocksWindowClass";

	if( !RegisterClassEx( &windowClass ) ) {
		OutputDebugStringA( "Failed to register window class!" );
		return 1;
	}

	RECT clientRect, desktopRect;
	HWND desktop = GetDesktopWindow();
	GetWindowRect( desktop, &desktopRect );
	if( Config.fullscreen || Config.screenWidth == 0 || Config.screenHeight == 0 )
	{
		clientRect = desktopRect;
	}
	else
	{
		clientRect.left = 10;
		clientRect.top = 30;
		clientRect.right = Config.screenWidth;
		clientRect.bottom = Config.screenHeight;
	}
	AdjustWindowRect( &clientRect, WS_OVERLAPPEDWINDOW, FALSE );

	HWND window = CreateWindow( "BlocksWindowClass", "Blocks", WS_OVERLAPPEDWINDOW, 
								clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
								NULL, NULL, thisInstance, NULL );
	if( !window ) {
		OutputDebugStringA( "Failed to create window!" );
		return 1;
	}

	ShowWindow( window, cmdShow );
	UpdateWindow( window );

	if( !RegisterRawInputDevices( window ) ) {
		OutputDebugStringA( "Failed to register raw input devices!" );
		return 1;
	}

	HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
	if( FAILED( hr ) ) {
		OutputDebugStringA( "Failed to show main window!" );
		return 1;
	}

	Game game = Game();
	if( !game.Start( window ) )
	{
		OutputDebugStringA( "Failed to start the game" );
		return 1;
	}

	/* pass a game pointer to the window for use in WndProc */
	SetWindowLongPtr( window, GWLP_USERDATA, (LONG_PTR)&game );

	/* Main loop */

	LONGLONG startTime = 0;
	LONGLONG endTime = 0;
	float delta = 0;

	MSG msg = { 0 };
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			// NOTE: using previous frame delta
			delta = (float)( Time::ToMiliseconds( endTime - startTime ) );
			startTime = Time::Now();
			game.DoFrame( delta );
			endTime = Time::Now();
		}
	}

	Profile::Report();

	return (int) msg.wParam;
}

// TODO: need better way to communicate with game
LRESULT CALLBACK WndProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT paintStruct;
	HDC hDC;

	Game *game = 0;

	switch( message )
	{
		case WM_INPUT:
			game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
			TranslateUserInput( game->input, lParam );
			break;
		case WM_KEYDOWN:
			switch( wParam )
			{
				case VK_ESCAPE:
					PostQuitMessage( 0 );
					break;
			}
			break;
		case WM_SYSKEYDOWN:
			switch( wParam )
			{
				case VK_RETURN:
					game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
					game->renderer.ToggleFullscreen();
					return 0;
					break;
			}
			break;
		case WM_ACTIVATEAPP:
			if( !wParam ) {
				game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
				if( game->renderer.Fullscreen() ) {
					game->renderer.ToggleFullscreen();
				}
			}
			break;
		case WM_EXITSIZEMOVE:
			if( wParam == SIZE_RESTORED )
			{
				game = (Game*)GetWindowLong( windowHandle, GWLP_USERDATA );
				if( game )
				{
					game->renderer.ResizeBuffers();
				}
				return 0;
			}
			break;
		case WM_PAINT:
			hDC = BeginPaint( windowHandle, &paintStruct );
			EndPaint( windowHandle, &paintStruct );
			break;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		default:
			return DefWindowProc( windowHandle, message, wParam, lParam );
	}
	return 0;
}

bool LoadConfig()
{
	// TODO: load config from file
	Config.fullscreen = FALSE;
	Config.vsync = TRUE;
	Config.screenWidth = 960;
	Config.screenHeight = 540;
//	Config.screenWidth = 1920;
//	Config.screenHeight = 1080;
	Config.multisampling = 0;

	return true;
}

/*
NOTE:
To get the list of raw input devices on the system,
an application calls GetRawInputDeviceList. 
Using the hDevice from this call, an application calls 
GetRawInputDeviceInfo to get the device information.
*/

bool RegisterRawInputDevices( HWND hwnd )
{
	RAWINPUTDEVICE inputDevices[2];
	inputDevices[ 0 ].usUsagePage = 1; // generic desktop controls
	inputDevices[ 0 ].usUsage = 2; // mouse
	inputDevices[ 0 ].dwFlags = 0;
	inputDevices[ 0 ].hwndTarget = hwnd;

	inputDevices[ 1 ].usUsagePage = 1; // generic desktop controls
	inputDevices[ 1 ].usUsage = 6; // keyboard
	inputDevices[ 1 ].dwFlags = 0;
	inputDevices[ 1 ].hwndTarget = hwnd;

	if( !RegisterRawInputDevices( inputDevices, 2, sizeof( RAWINPUTDEVICE ) ) )
	{
		return false;
	}
	return true;
}

// Use windows VK codes for now
void TranslateUserInput( UserInput &userInput, LPARAM lParam )
{
	// get the required buffer size
	UINT size;
	GetRawInputData( (HRAWINPUT)lParam,
					 RID_INPUT,
					 NULL,
					 &size,
					 sizeof( RAWINPUTHEADER ) );

	LPBYTE buffer = new BYTE[ size ];

	UINT bytesRead = GetRawInputData( (HRAWINPUT)lParam,
									  RID_INPUT,
									  buffer,
									  &size,
									  sizeof( RAWINPUTHEADER ) );
	if( bytesRead != size )
	{
		OutputDebugStringA( "GetRawInputData returned incorrect size!" );
	}

	RAWINPUT* input = (RAWINPUT*)buffer;

	if( input->header.dwType == RIM_TYPEMOUSE )
	{
		long x = input->data.mouse.lLastX;
		long y = input->data.mouse.lLastY;

		userInput.mouse.x += x;
		userInput.mouse.y += y;

		return;
	}
	else if( input->header.dwType == RIM_TYPEKEYBOARD )
	{
		unsigned short flags = input->data.keyboard.Flags;
//		unsigned short scancode = input->data.keyboard.MakeCode;
		unsigned short key = input->data.keyboard.VKey;
		unsigned short keyExtra = 0; // left or right

		bool pressed = true;

		if( ( flags & RI_KEY_BREAK ) == RI_KEY_BREAK )
		{
			pressed = false;
		}

		if( key == VK_CONTROL )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 ) // should be E1!
			{
				keyExtra = VK_RCONTROL;
			}
			else
			{
				keyExtra = VK_LCONTROL;
			}
		}
		else if( key == VK_MENU )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 )
			{
				keyExtra = VK_RMENU;
			}
			else
			{
				keyExtra = VK_LMENU;
			}
		}
		else if( key == VK_RETURN )
		{
			if( ( flags & RI_KEY_E0 ) == RI_KEY_E0 )
			{
				keyExtra = VK_NUMPAD_ENTER;
			}
			else
			{
				keyExtra = VK_RETURN;
			}
		}
		else if( key == VK_SHIFT )
		{
			if( input->data.keyboard.MakeCode == 0x36 )
			{
				keyExtra = VK_RSHIFT;
			}
			else
			{
				keyExtra = VK_LSHIFT;
			}
		}
		userInput.key[ key ] =  pressed;
		if( keyExtra )
		{
			userInput.key[ keyExtra ] = pressed;
		}
	}

	delete[] buffer;
}