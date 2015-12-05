#include "tokenstream.h"

static bool IsDelimitingChar( char c, const char *delimiters );

TokenStream::TokenStream()
{
	buffer_ = 0;
	length_ = 0;
	position_ = 0;
}

TokenStream::TokenStream( const char *buffer, unsigned int length )
{
	SetSource( buffer, length );
}

TokenStream::~TokenStream()
{
	buffer_ = 0;
}

unsigned int TokenStream::NextToken( char *buffer, const char *delimiters )
{
	// skip delim and empty lines
	while( position_ < length_ && IsDelimitingChar( buffer_[ position_ ], delimiters ) )
	{
		++position_;
	}

	unsigned int start = position_;
	while( position_ < length_ && !IsDelimitingChar( buffer_[ position_ ], delimiters ) )
	{
		++position_;
	}

	if( position_ == start )
	{
		return 0;
	}

	assert( (position_ - start + 1) <= _MAX_TOKEN_LENGTH_ );

	memcpy( buffer, &buffer_[ start ], position_ - start );
	buffer[ position_ - start ] = '\0';

	return position_ - start + 1;
}

void TokenStream::Reset()
{
	position_ = 0;
}

void TokenStream::SetSource( const char *buffer, unsigned int length )
{
	buffer_ = buffer;
	length_ = length;
	position_ = 0;
}

static bool IsDelimitingChar( char c, const char *delimiters )
{
	// check implicit delimiters
	if( c == '\n' || c == '\r' || c == '\0' )
	{
		return true;
	}
	// check specified delimiters
	unsigned int length = (unsigned int)strlen( delimiters );
	for( unsigned int i = 0; i < length; i++ )
	{
		if( c == delimiters[i] )
		{
			return true;
		}
	}

	return false;
}
