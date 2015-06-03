#ifndef __BLOCKS_UTILS__
#define __BLOCKS_UTILS__

#include <Windows.h>
#include <assert.h>
#include <cstdio>

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

#define MAX_PROFILES 32
class Profile
{
public:
	static void Start( const char* name );
	static void Stop();
	// call this on program exit
	// TODO: auto-reporting
	static void Report();
private:
	struct ProfileEntry {
		const char* name;
		long long time;
	};

	static int activeProfileIndex_; // last active profile + 1
	static ProfileEntry activeProfiles_[ MAX_PROFILES ]; // active profiles in LIFO order
	static int storeProfileIndex_; // next free slot
	static ProfileEntry finishedProfiles_[ MAX_PROFILES ]; // finished profiles - grow only
};

#endif //__BLOCKS_UTILS__