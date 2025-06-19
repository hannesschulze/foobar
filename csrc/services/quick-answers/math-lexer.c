#include "services/quick-answers/math.h"

//
// FoobarMathToken:
//
// A token in a mathematical expression. The token contents are left as-is and need not be null-terminated. For example,
// "sin(pi) - 3" would be represented as:
//  - IDENTIFIER: "sin"
//  - PAREN_OPEN: "("
//  - IDENTIFIER: "pi"
//  - PAREN_CLOSE: ")"
//  - MINUS: "-"
//  - NUMBER: "3"
//

//
// Lexer:
//
// The lexer context/state which is passed around.
//

typedef struct _Lexer Lexer;

struct _Lexer
{
	GArray*      result;
	gchar const* input;
	gsize        input_length;
	gsize        position;
	gsize        token_start;
};

static gboolean char_is_whitespace( char c );
static gboolean char_is_digit     ( char c );
static gboolean char_is_identifier( char c );

static char lexer_peek         ( Lexer const*        ctx,
                                 gsize               offset );
static char lexer_pop          ( Lexer*              ctx );
static void lexer_begin_token  ( Lexer*              ctx );
static void lexer_commit_token ( Lexer*              ctx,
                                 FoobarMathTokenType type );
static void lexer_commit_single( Lexer*              ctx,
                                 FoobarMathTokenType type );

// ---------------------------------------------------------------------------------------------------------------------
// Lexing
// ---------------------------------------------------------------------------------------------------------------------

//
// Divide an input string into a list of tokens.
//
// On success, this will return a newly allocated array of tokens which should be freed using g_free. The number of
// elements will be written into out_count.
//
// On error, this will return NULL.
//
FoobarMathToken* foobar_math_lex(
	gchar const* input,
	gsize        input_length,
	gsize*       out_count )
{
	g_autoptr( GArray ) result = g_array_new( FALSE, TRUE, sizeof( FoobarMathToken ) );

	Lexer ctx = { 0 };
	ctx.result = result;
	ctx.input = input;
	ctx.input_length = input_length;

	while ( ctx.position < input_length )
	{
		if ( char_is_whitespace( lexer_peek( &ctx, 0 ) ) )
		{
			// Skip whitespace.

			lexer_pop( &ctx );
		}
		else if ( char_is_digit( lexer_peek( &ctx, 0 ) ) )
		{
			// Process numbers as a whole, don't allow whitespace inbetween.

			lexer_begin_token( &ctx );
			while ( char_is_digit( lexer_peek( &ctx, 0 ) ) )
			{
				lexer_pop( &ctx );
			}

			if ( lexer_peek( &ctx, 0 ) == '.' && char_is_digit( lexer_peek( &ctx, 1 ) ) )
			{
				lexer_pop( &ctx );
				while ( char_is_digit( lexer_peek( &ctx, 0 ) ) )
				{
					lexer_pop( &ctx );
				}
			}

			lexer_commit_token( &ctx, FOOBAR_MATH_TOKEN_NUMBER );
		}
		else if ( char_is_identifier( lexer_peek( &ctx, 0 ) ) )
		{
			// Don't validate identifiers, leave that to the parser.

			lexer_begin_token( &ctx );
			while ( char_is_identifier( lexer_peek( &ctx, 0 ) ) )
			{
				lexer_pop( &ctx );
			}
			lexer_commit_token( &ctx, FOOBAR_MATH_TOKEN_IDENTIFIER );
		}
		else if ( lexer_peek( &ctx, 0 ) == '(' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_PAREN_OPEN );
		}
		else if ( lexer_peek( &ctx, 0 ) == ')' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_PAREN_CLOSE );
		}
		else if ( lexer_peek( &ctx, 0 ) == '+' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_PLUS );
		}
		else if ( lexer_peek( &ctx, 0 ) == '-' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_MINUS );
		}
		else if ( lexer_peek( &ctx, 0 ) == '*' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_MUL );
		}
		else if ( lexer_peek( &ctx, 0 ) == '/' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_DIV );
		}
		else if ( lexer_peek( &ctx, 0 ) == '^' )
		{
			lexer_commit_single( &ctx, FOOBAR_MATH_TOKEN_POW );
		}
		else
		{
			// Unknown character.

			return NULL;
		}
	}

	return g_array_steal( result, out_count );
}

// ---------------------------------------------------------------------------------------------------------------------
// Character Classification
// ---------------------------------------------------------------------------------------------------------------------

//
// Check if a character is a whitespace symbol (space, tab, carriage return).
//
gboolean char_is_whitespace( char c )
{
	return c == ' ' || c == '\t' || c == '\r';
}

//
// Check if a character is a decimal digit.
//
gboolean char_is_digit( char c )
{
	return c >= '0' && c <= '9';
}

//
// Check if a character belongs to an identifier (like "sin", "pi", etc.).
//
gboolean char_is_identifier( char c )
{
	// Identifiers are all lowercase ASCII.

	return c >= 'a' && c <= 'z';
}

// ---------------------------------------------------------------------------------------------------------------------
// State Helpers
// ---------------------------------------------------------------------------------------------------------------------

//
// Peek at the current character without consuming it.
//
// If this is the end of the input string, return 0.
//
char lexer_peek(
	Lexer const* ctx,
	gsize        offset )
{
	gsize position = ctx->position + offset;
	if ( position >= ctx->input_length ) { return 0; }

	return ctx->input[position];
}

//
// Consume the current character, moving forward to the next one.
//
// If this is the end of the input string, return 0.
//
char lexer_pop( Lexer* ctx )
{
	if ( ctx->position >= ctx->input_length ) { return 0; }

	return ctx->input[ctx->position++];
}

//
// Start recording a token by remembering the current position.
//
// The current character will be the first one in the token.
//
void lexer_begin_token( Lexer* ctx )
{
	ctx->token_start = ctx->position;
}

//
// Stop recording a token, saving the string between the call to lexer_begin_token as the specified type.
//
// The current character will not be a part of the token.
//
void lexer_commit_token(
	Lexer*              ctx,
	FoobarMathTokenType type )
{
	FoobarMathToken token = { 0 };
	token.data = &ctx->input[ctx->token_start];
	token.length = ctx->position - ctx->token_start;
	token.type = type;
	g_array_append_val( ctx->result, token );
}

//
// Consume the current character and record it as a token.
//
// This is equivalent to calling lexer_begin_token, lexer_pop, and lexer_commit_token.
//
void lexer_commit_single(
	Lexer*              ctx,
	FoobarMathTokenType type )
{
	lexer_begin_token( ctx );
	lexer_pop( ctx );
	lexer_commit_token( ctx, type );
}
