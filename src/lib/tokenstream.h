#ifndef __TOKENSTREAM__
#define __TOKENSTREAM__

//--------------------------------------
// Represents up a char buffer as a srteam
// of tokens, using a delimiter.
// Delimiter is consumed.
// Newline and null-char are considered
// delimiters by default.
//--------------------------------------

#include <cstring>
#include <assert.h>

#define _MAX_TOKEN_LENGTH_ 512

// TODO: unified type system
// TODO: should I assume, that all buffers are C-strings and drop length_?

class TokenStream
{
public:
	TokenStream();
	TokenStream(const char *buffer, unsigned int length );
	~TokenStream();

	// writes the next token to buffer
	// returns the length of the token
	// or 0, when the internal buffer
	// doesn't have any tokens left
	unsigned int NextToken( char *buffer, const char *delimiters = "\n" );

	// sets position_ to 0
	void Reset();

	// sets the new char buffer as a stream source
	// and reset position_ to 0
	// The user *must* set the appropriate length,
	// unless buffer is a null-terminated string
	void SetSource( const char *buffer, unsigned int length = _MAX_TOKEN_LENGTH_ );
private:
	const char *buffer_;
	unsigned int length_;
	unsigned int position_;
};

#endif
