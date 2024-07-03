#include "services/quick-answers/math.h"

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

gboolean char_is_whitespace( char c )
{
	return c == ' ' || c == '\t' || c == '\r';
}

gboolean char_is_digit( char c )
{
	return c >= '0' && c <= '9';
}

gboolean char_is_identifier( char c )
{
	// Identifiers are all lowercase ASCII.

	return c >= 'a' && c <= 'z';
}

char lexer_peek(
	Lexer const* ctx,
	gsize        offset )
{
	gsize position = ctx->position + offset;
	if ( position >= ctx->input_length ) { return 0; }

	return ctx->input[position];
}

char lexer_pop( Lexer* ctx )
{
	if ( ctx->position >= ctx->input_length ) { return 0; }

	return ctx->input[ctx->position++];
}

void lexer_begin_token( Lexer* ctx )
{
	ctx->token_start = ctx->position;
}

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

void lexer_commit_single(
	Lexer*              ctx,
	FoobarMathTokenType type )
{
	lexer_begin_token( ctx );
	lexer_pop( ctx );
	lexer_commit_token( ctx, type );
}
