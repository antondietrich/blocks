#include "utils.h"

float Saturate( float f )
{
	if( f < 0.0f )
	{
		return 0.0f;
	}
	if( f > 1.0f )
	{
		return 1.0f;
	}
	return f;
}

float Clamp( float f, float min, float max )
{
	if( f < min )
	{
		return min;
	}
	if( f > max )
	{
		return max;
	}
	return f;
}

LONGLONG GetTicksPerSecond();

LONGLONG GetTicksPerSecond();

LONGLONG Time::ticksPerSecond_ = GetTicksPerSecond();

LONGLONG Time::Now()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter( &time );
	return time.QuadPart;
}

double Time::ToSeconds( LONGLONG ticks )
{
	assert( Time::ticksPerSecond_ );
	return (double)ticks / Time::ticksPerSecond_;
}

double Time::ToMiliseconds( LONGLONG ticks )
{
	assert( Time::ticksPerSecond_ );
	return ( (double)ticks / Time::ticksPerSecond_ ) * 1000.0;
}

LONGLONG GetTicksPerSecond()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency( &frequency );
	return frequency.QuadPart;
}

//********************
// Profile
//********************
int64 Profile::framesRendered_;
float Profile::avgFrameTime_ = 0;
int Profile::activeProfileIndex_ = 0;
Profile::ProfileEntry Profile::activeProfiles_[];
Profile::ProfileEntry Profile::finishedProfiles_[];

// TODO: better hash function XD
uint8 hash( const uint8 *str );

float Average( float a, float b )
{
	float result;
	if( a == 0 ) {
		result = b;
	}
	result = ( a + b ) / 2.0f;
	return result;
}

int64 Average( int64 a, int64 b )
{
	int64 result;
	if( a == 0 ) {
		result = b;
	}
	result = ( a + b ) / 2;
	return result;
}

void Profile::NewFrame( float frameTime )
{
	framesRendered_++;
	avgFrameTime_ = Average( avgFrameTime_, frameTime );

	for( int i = 0; i < MAX_STORED_PROFILES; i++ )
	{
		if( finishedProfiles_[i].callsPerFrame != 0 )
		{
			finishedProfiles_[i].timePerCall = Average( finishedProfiles_[i].timePerCall, 
						finishedProfiles_[i].timePerCallAccumulator / finishedProfiles_[i].callsPerFrame );
			finishedProfiles_[i].timePerFrame = Average( finishedProfiles_[i].timePerFrame,
														 finishedProfiles_[i].timePerCallAccumulator );
			finishedProfiles_[i].timePerCallAccumulator = 0;
			finishedProfiles_[i].callsPerFrame = 0;
		}
	}
}

void Profile::Start( const char* name )
{
	assert( activeProfileIndex_ < MAX_ACTIVE_PROFILES );
	activeProfiles_[ activeProfileIndex_ ].name = name;
	activeProfiles_[ activeProfileIndex_ ].timePerCall = Time::Now();

	activeProfileIndex_++;
}

void Profile::Stop()
{
	// pop the last active profile
	activeProfileIndex_--;

	int64 profileTime = Time::Now() - activeProfiles_[ activeProfileIndex_ ].timePerCall;

	uint8 storageIndex = hash( (uint8*)activeProfiles_[ activeProfileIndex_ ].name );

	// make sure we didn't hit a collision
	assert( finishedProfiles_[ storageIndex ].name == 0 || 
		0 == strcmp( finishedProfiles_[ storageIndex ].name, activeProfiles_[ activeProfileIndex_ ].name ) );


	finishedProfiles_[ storageIndex ].name = activeProfiles_[ activeProfileIndex_ ].name;
	finishedProfiles_[ storageIndex ].timePerCallAccumulator += profileTime;
	finishedProfiles_[ storageIndex ].callsPerFrame++;
}

void Profile::Report()
{
	OutputDebugStringA( "\n\n*** Profiler report ***\n" );

	OutputDebugStringA( "Frames rendered: " );
	OutputDebugStringA( std::to_string( framesRendered_ ).c_str() );
	OutputDebugStringA( "\n" );

	OutputDebugStringA( "Average frame time: " );
	OutputDebugStringA( std::to_string( avgFrameTime_ ).c_str() );
	OutputDebugStringA( "\n" );

	OutputDebugStringA( "Name                             | per call | per frame |   %  \n" );
	OutputDebugStringA( "---------------------------------|----------|-----------|------\n" );
	for( int i = 0; i < MAX_STORED_PROFILES; i++ )
	{
		if( finishedProfiles_[i].name == 0 )
			continue;

		finishedProfiles_[i].percentage = (float)( Time::ToMiliseconds( finishedProfiles_[i].timePerFrame ) / avgFrameTime_ ) * 100.0f;
		char report[256];
		sprintf( report, "%-32s | %8.4f | %9.4f | %5.2f\n", finishedProfiles_[i].name,
				 Time::ToMiliseconds( finishedProfiles_[i].timePerCall ),
				 Time::ToMiliseconds( finishedProfiles_[i].timePerFrame ),
				 finishedProfiles_[i].percentage );

		OutputDebugStringA( report );
	}
	OutputDebugStringA( "\n\n" );
}

#pragma warning( disable : 4706 )

uint8 hash( const uint8 *str )
{
	//unsigned long hash = 5381;
	uint8 hash = 179;
	uint8 c;

	while( c = *str++ )
		hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */

	return hash;
}

#pragma warning( default : 4706 )