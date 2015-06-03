#include "utils.h"

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
int Profile::activeProfileIndex_ = 0;
int Profile::storeProfileIndex_ = 0;
Profile::ProfileEntry Profile::activeProfiles_[];
Profile::ProfileEntry Profile::finishedProfiles_[];

void Profile::Start( const char* name )
{
	assert( activeProfileIndex_ < MAX_PROFILES );
	activeProfiles_[ activeProfileIndex_ ].name = name;
	activeProfiles_[ activeProfileIndex_ ].time = Time::Now();
	activeProfileIndex_++;
}

void Profile::Stop()
{
	activeProfileIndex_--;

	long long profileTime = Time::Now() - activeProfiles_[ activeProfileIndex_ ].time;

	assert( storeProfileIndex_ < MAX_PROFILES );

	if( finishedProfiles_[ storeProfileIndex_ ].time == 0) {
		finishedProfiles_[ storeProfileIndex_ ].name = activeProfiles_[ activeProfileIndex_ ].name;
		finishedProfiles_[ storeProfileIndex_ ].time = profileTime;
	}
	else
	{
		finishedProfiles_[ storeProfileIndex_ ].time = ( finishedProfiles_[ storeProfileIndex_ ].time + profileTime ) / 2;
	}

	storeProfileIndex_++;

	if( activeProfileIndex_ == 0 ) // all profilers stopped
	{
		storeProfileIndex_ = 0;
	}
}

void Profile::Report()
{
	OutputDebugStringA( "\n*** Profiler report ***\n" );
	for( int i = 0; i < MAX_PROFILES; i++ )
	{
		if( finishedProfiles_[i].time == 0 )
			continue;
		char report[256];
		sprintf( report, "%s: %6.3f\n", finishedProfiles_[i].name, Time::ToMiliseconds( finishedProfiles_[i].time ) );
		OutputDebugStringA( report );
	}
}