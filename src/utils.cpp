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
float Profile::avgFrameTime_ = 0;
Profile::ProfileEntry Profile::activeProfiles_[];
Profile::ProfileEntry Profile::finishedProfiles_[];

void Profile::NewFrame( float frameTime )
{
	if( avgFrameTime_ ==  0 ) {
		avgFrameTime_ = frameTime;
	}
	else {
		avgFrameTime_ = ( avgFrameTime_ + frameTime ) / 2;
	}
}

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

	// same name with the previous record, must be a loop
	if( storeProfileIndex_ != 0 && 
		0 == strcmp( activeProfiles_[ activeProfileIndex_ ].name, finishedProfiles_[ storeProfileIndex_ - 1 ].name ) )
	{
		storeProfileIndex_--;
	}

	long long profileTime = Time::Now() - activeProfiles_[ activeProfileIndex_ ].time;
	
	finishedProfiles_[ storeProfileIndex_ ].callsPerFrame++;
	finishedProfiles_[ storeProfileIndex_ ].frameTime += profileTime;


	assert( storeProfileIndex_ < MAX_PROFILES );

	if( finishedProfiles_[ storeProfileIndex_ ].time == 0) { // first frame
		finishedProfiles_[ storeProfileIndex_ ].time = profileTime;
		finishedProfiles_[ storeProfileIndex_ ].name = activeProfiles_[ activeProfileIndex_ ].name;
	}
	else {
		finishedProfiles_[ storeProfileIndex_ ].time = ( finishedProfiles_[ storeProfileIndex_ ].time + profileTime ) / 2;
	}

	storeProfileIndex_++;

	// FIX: can't have more than one upper level profiler!
	if( activeProfileIndex_ == 0 ) // all profilers stopped
	{
		storeProfileIndex_ = 0;
		for( int i = 0; i < MAX_PROFILES; i++ )
		{
			if( finishedProfiles_[i].callsPerFrame != 0 )
			{
				long long thisFrameTime = finishedProfiles_[i].frameTime;
				if( finishedProfiles_[i].avgFrameTime == 0 ) { // first frame
					finishedProfiles_[i].avgFrameTime = thisFrameTime;
				}
				else {
					finishedProfiles_[i].avgFrameTime = ( finishedProfiles_[i].avgFrameTime + thisFrameTime ) / 2;
				}
			}
			finishedProfiles_[i].frameTime = 0;
		}
	}
}

void Profile::Report()
{
	OutputDebugStringA( "\n\n*** Profiler report ***\n" );
	OutputDebugStringA( "Name                             | per call | per frame |   %  \n" );
	OutputDebugStringA( "---------------------------------|----------|-----------|------\n" );
	for( int i = 0; i < MAX_PROFILES; i++ )
	{
		if( finishedProfiles_[i].time == 0 )
			continue;
		finishedProfiles_[i].percentage = ( Time::ToMiliseconds( finishedProfiles_[i].avgFrameTime ) / avgFrameTime_ ) * 100;
		char report[256];
		sprintf( report, "%-32s | %8.4f | %9.4f | %5.2f\n", finishedProfiles_[i].name,
				 Time::ToMiliseconds( finishedProfiles_[i].time ),
				 Time::ToMiliseconds( finishedProfiles_[i].avgFrameTime ),
				 finishedProfiles_[i].percentage );

		OutputDebugStringA( report );
	}
}