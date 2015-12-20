#ifndef __BLOCKS_UTILS__
#define __BLOCKS_UTILS__

#include <Windows.h>
#include <assert.h>
#include <cstdio>
#include <string>

#include "types.h"

//
// Safely releases a COM interface
//
#define RELEASE(p) if(p) { p->Release(); p = 0; }

//
// Calculates the number of elements in a static array
//
#define ARRAY_SIZE(a) ( sizeof( a ) / sizeof( a[0] ) )

//
// Useful math
//
float Saturate( float f );
float Clamp( float f, float min, float max );
inline float Lerp( float a, float b, float t )
{
	return a * ( 1 - t ) + b * t;
}


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


//
// Profiler
//
#ifdef _PROFILE_

#define ProfileStart( name ) Profile::Start( (name) );
#define ProfileStop() Profile::Stop();
#define ProfileNewFrame( dt ) Profile::NewFrame( (dt) );
#define ProfileReport()  Profile::Report();

#else

#define ProfileStart( name )
#define ProfileStop()
#define ProfileNewFrame( dt )
#define ProfileReport()

#endif

#define MAX_ACTIVE_PROFILES 32
#define MAX_STORED_PROFILES 255
class Profile
{
public:
	static void NewFrame( float frameTime );
	static void Start( const char* name );
	static void Stop();
	// call this on program exit
	// TODO: auto-reporting
	static void Report();
private:
	struct ProfileEntry {
		const char* name;
		int64 timePerCall;
		int64 timePerCallAccumulator;
		int64 timePerFrame;
		uint callsPerFrame;
		float percentage;
	};

	static int64 framesRendered_;
	static float avgFrameTime_;
	static int activeProfileIndex_; // last active profile + 1
	static ProfileEntry activeProfiles_[ MAX_ACTIVE_PROFILES ]; // active profiles in LIFO order
	static ProfileEntry finishedProfiles_[ MAX_STORED_PROFILES ]; // finished profiles - grow only
};

#endif //__BLOCKS_UTILS__
