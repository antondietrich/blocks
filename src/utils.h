#ifndef __BLOCKS_UTILS__
#define __BLOCKS_UTILS__

#include <Windows.h>
#include <assert.h>

//
// Safely releases a COM interface
//
#define RELEASE(p) if(p) { p->Release(); p = 0; }

//
// Hi-res timer
//
class Time
{
public:
	static LONGLONG Now();
	static double ToSeconds( LONGLONG ticks );
	static double ToMiliseconds( LONGLONG ticks );
private:
	static LONGLONG ticksPerSecond_;
};

#endif //__BLOCKS_UTILS__