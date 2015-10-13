#ifndef __BLOCKS_INPUT__
#define __BLOCKS_INPUT__

#include "types.h"

namespace Blocks
{


enum /*class*/ KEY : uint8;

struct KeyState
{
	bool Down;
	bool Pressed;
	bool Released;
};

//const char* gKeyName[];
//KEY gKeyValuesForVKeys[];
KEY VKeyToKey( unsigned short vKey );
const char* GetKeyName( KEY key );

enum /*class*/ KEY : uint8
{
	UNDEF = 0,
	ESC = 1,
	TAB, CAPS, LSHIFT, RSHIFT, LCTRL, RCTRL,
	LALT, RALT, LWIN, RWIN, SPACE, BACKSPACE,
	ENTER, // 14
	
	TILDE, // 15
	SLASH, BACKSLASH, VLINE_, LBRACKET, RBRACKET, COMMA,
	PERIOD, SEMICOLON, QUOTE, HYPHEN, 
	EQUAL, // 26

	F1, // 27
	F2,	F3,	F4,	F5,	F6,
	F7,	F8,	F9,	F10, F11,
	F12, // 38

	PRTSCR, // 39
	SCROLL, PAUSE, INSERT, DEL, 
	HOME, END, PAGEUP,
	PAGEDOWN, // 47

	ZERO = 48, // '0'
	ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, 
	NINE, // 57

	A = 65, // 'A'
	B, C, D, E, F, 
	G, H, I, J, K,
	L, M, N, O, P,
	Q, R, S, T, U,
	V, W, X, Y, 
	Z, // 90

	UP, DOWN, LEFT, RIGHT,

	SHIFT, CTRL, ALT, WIN,

	VOL_UP, VOL_DOWN, VOL_MUTE,

	NUM,
	NUM_1, NUM_2, NUM_3, NUM_4, NUM_5,
	NUM_6, NUM_7, NUM_8, NUM_9, NUM_0,
	NUM_DIV, NUM_MUL, NUM_ADD, NUM_SUB,
	NUM_ENTER, NUM_DECIMAL, NUM_SEPARATOR,

	LMB, RMB, MMB, MB4, MB5, MBSCROLLUP, MBSCROLLDOWN,


	COUNT
};

struct UserInput
{
	KeyState key[ KEY::COUNT ];
	struct {
		long x;
		long y;
	} mouse;
};

}

#endif // __BLOCKS_INPUT__
