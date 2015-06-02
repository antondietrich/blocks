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