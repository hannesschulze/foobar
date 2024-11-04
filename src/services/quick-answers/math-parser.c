#include "services/quick-answers/math.h"

//
// Parser:
//
// The parser context/state which is passed around.
//

typedef struct _Parser Parser;

struct _Parser
{
	FoobarMathToken const* tokens;
	gsize                  tokens_count;
	gsize                  position;
};

//
// InfixPrecedence:
//
// Value indicating how strongly infix operators should bind. The higher the value, the stronger they should bind.
//

typedef enum
{
	INFIX_PRECEDENCE_INVALID        = -1,
	INFIX_PRECEDENCE_NONE           = 0,
	INFIX_PRECEDENCE_ADDITIVE       = 1,
	INFIX_PRECEDENCE_MULTIPLICATIVE = 2,
	INFIX_PRECEDENCE_EXPONENTS      = 3,
} InfixPrecedence;

static FoobarMathExpression*  parser_process            ( Parser*                ctx,
                                                          FoobarMathExpression*  lhs,
                                                          InfixPrecedence        min_precedence );
static FoobarMathExpression*  parser_process_single     ( Parser*                ctx,
                                                          gboolean               allow_sign );
static FoobarMathExpression*  parser_process_identifier ( Parser*                ctx );
static FoobarMathExpression*  parser_process_number     ( Parser*                ctx );
static FoobarMathExpression*  parser_process_function   ( Parser*                ctx,
                                                          FoobarMathFunction     function );
static FoobarMathExpression*  parser_process_parens     ( Parser*                ctx );
static FoobarMathExpression*  parser_process_sign       ( Parser*                ctx );
static FoobarMathToken const* parser_peek               ( Parser const*          ctx );
static FoobarMathToken const* parser_pop                ( Parser*                ctx );
static gboolean               parser_match_identifier   ( FoobarMathToken const* token,
                                                          gchar const*           identifier );
static InfixPrecedence        parser_operator_precedence( FoobarMathToken const* token );
static FoobarMathOperation    parser_operator           ( FoobarMathToken const* token );

// ---------------------------------------------------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------------------------------------------------

//
// Transform a sequence of tokens into a mathematical expression.
//
// On success, this will return a newly allocated expression which should be freed using foobar_math_expression_free.
//
// On error, this will return NULL.
//
FoobarMathExpression* foobar_math_parse(
	FoobarMathToken const* tokens,
	gsize                  tokens_count )
{
	Parser ctx = { 0 };
	ctx.tokens = tokens;
	ctx.tokens_count = tokens_count;

	FoobarMathExpression* lhs = parser_process_single( &ctx, TRUE );
	if ( !lhs ) { return NULL; }

	FoobarMathExpression* expr = parser_process( &ctx, lhs, INFIX_PRECEDENCE_NONE );
	if ( !expr ) { return NULL; }

	if ( parser_peek( &ctx ) )
	{
		foobar_math_expression_free( expr );
		return NULL;
	}

	return expr;
}

//
// Process tokens until the end of the current precedence scope (indicated by min_precedence), starting at an operator.
//
FoobarMathExpression* parser_process(
	Parser*               ctx,
	FoobarMathExpression* lhs,
	InfixPrecedence       min_precedence )
{
	FoobarMathToken const* op = parser_peek( ctx );
	InfixPrecedence op_precedence = parser_operator_precedence( op );
	while ( op_precedence >= min_precedence )
	{
		parser_pop( ctx );

		FoobarMathExpression* rhs = parser_process_single( ctx, FALSE );
		if ( !rhs )
		{
			foobar_math_expression_free( lhs );
			return NULL;
		}

		FoobarMathToken const* next_op = parser_peek( ctx );
		while ( parser_operator_precedence( next_op ) > op_precedence )
		{
			rhs = parser_process( ctx, rhs, op_precedence + 1 );
			if ( !rhs )
			{
				foobar_math_expression_free( lhs );
				return NULL;
			}

			next_op = parser_peek( ctx );
		}

		lhs = foobar_math_expression_new_operation( parser_operator( op ), lhs, rhs );

		op = parser_peek( ctx );
		op_precedence = parser_operator_precedence( op );
	}

	return lhs;
}

//
// Process a single value which may be either a constant, function, number, expression enclosed in parenthesis or
// negation.
//
// If an unexpected token is encountered (i.e. an operator), this will return FALSE.
//
FoobarMathExpression* parser_process_single(
	Parser*  ctx,
	gboolean allow_sign )
{
	FoobarMathToken const* token = parser_peek( ctx );
	if ( !token ) { return NULL; }

	switch ( token->type )
	{
		case FOOBAR_MATH_TOKEN_IDENTIFIER:
			return parser_process_identifier( ctx );
		case FOOBAR_MATH_TOKEN_NUMBER:
			return parser_process_number( ctx );
		case FOOBAR_MATH_TOKEN_PAREN_OPEN:
			return parser_process_parens( ctx );
		case FOOBAR_MATH_TOKEN_MINUS:
		case FOOBAR_MATH_TOKEN_PLUS:
			if ( !allow_sign ) { return NULL; }
			return parser_process_sign( ctx );
		case FOOBAR_MATH_TOKEN_PAREN_CLOSE:
		case FOOBAR_MATH_TOKEN_MUL:
		case FOOBAR_MATH_TOKEN_DIV:
		case FOOBAR_MATH_TOKEN_POW:
			return NULL;
		default:
			g_warn_if_reached( );
			return NULL;
	}
}

//
// Parse an identifier.
//
// If the identifier is a function, this will expected parenthesis and an expression afterwards.
//
FoobarMathExpression* parser_process_identifier( Parser* ctx )
{
	FoobarMathToken const* token = parser_pop( ctx );
	if ( parser_match_identifier( token, "pi" ) )
	{
		return foobar_math_expression_new_constant( FOOBAR_MATH_CONSTANT_PI );
	}
	else if ( parser_match_identifier( token, "e" ) )
	{
		return foobar_math_expression_new_constant( FOOBAR_MATH_CONSTANT_E );
	}
	else if ( parser_match_identifier( token, "exp" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_EXP );
	}
	else if ( parser_match_identifier( token, "sqrt" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_SQRT );
	}
	else if ( parser_match_identifier( token, "sin" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_SIN );
	}
	else if ( parser_match_identifier( token, "cos" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_COS );
	}
	else if ( parser_match_identifier( token, "sec" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_SEC );
	}
	else if ( parser_match_identifier( token, "csc" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_CSC );
	}
	else if ( parser_match_identifier( token, "tan" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_TAN );
	}
	else if ( parser_match_identifier( token, "cot" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_COT );
	}
	else if ( parser_match_identifier( token, "arcsin" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCSIN );
	}
	else if ( parser_match_identifier( token, "arccos" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCCOS );
	}
	else if ( parser_match_identifier( token, "arcsec" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCSEC );
	}
	else if ( parser_match_identifier( token, "arccsc" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCCSC );
	}
	else if ( parser_match_identifier( token, "arctan" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCTAN );
	}
	else if ( parser_match_identifier( token, "arccot" ) )
	{
		return parser_process_function( ctx, FOOBAR_MATH_FUNCTION_ARCCOT );
	}
	else
	{
		return NULL;
	}
}

//
// Process a numeric value.
//
FoobarMathExpression* parser_process_number( Parser* ctx )
{
	FoobarMathToken const* token = parser_pop( ctx );
	FoobarMathValue value = { 0 };
	if ( !foobar_math_value_from_string( token->data, token->length, &value ) )
	{
		return NULL;
	}

	return foobar_math_expression_new_value( value );
}

//
// Process a function's parameter which should be enclosed in parenthesis.
//
FoobarMathExpression* parser_process_function(
	Parser*            ctx,
	FoobarMathFunction function )
{
	FoobarMathToken const* opening_paren = parser_peek( ctx );
	if ( !opening_paren || opening_paren->type != FOOBAR_MATH_TOKEN_PAREN_OPEN )
	{
		return NULL;
	}

	FoobarMathExpression* arg = parser_process_parens( ctx );
	if ( !arg ) { return NULL; }

	return foobar_math_expression_new_function( function, arg );
}

//
// Process an expression enclosed in parenthesis.
//
FoobarMathExpression* parser_process_parens( Parser* ctx )
{
	parser_pop( ctx );
	FoobarMathExpression* lhs = parser_process_single( ctx, TRUE );
	if ( !lhs ) { return NULL; }
	FoobarMathExpression* result = parser_process( ctx, lhs, INFIX_PRECEDENCE_NONE );
	if ( !result ) { return NULL; }
	FoobarMathToken const* closing_paren = parser_pop( ctx );
	if ( !closing_paren || closing_paren->type != FOOBAR_MATH_TOKEN_PAREN_CLOSE )
	{
		foobar_math_expression_free( result );
		return NULL;
	}
	return result;
}

//
// Process a sign (- or +) which may be used for negation of an expression.
//
FoobarMathExpression* parser_process_sign( Parser* ctx )
{
	FoobarMathToken const* token = parser_pop( ctx );
	FoobarMathExpression* result = parser_process_single( ctx, FALSE );
	if ( !result ) { return NULL; }

	if ( token->type == FOOBAR_MATH_TOKEN_MINUS )
	{
		return foobar_math_expression_new_function( FOOBAR_MATH_FUNCTION_NEGATE, result );
	}
	else
	{
		return result;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// State Helpers
// ---------------------------------------------------------------------------------------------------------------------

//
// Peek at the current token without consuming it.
//
// If this is the end of the token list, return NULL.
//
FoobarMathToken const* parser_peek( Parser const* ctx )
{
	if ( ctx->position >= ctx->tokens_count ) { return NULL; }

	return &ctx->tokens[ctx->position];
}

//
// Consume the current token.
//
// If this is the end of the token list, return NULL.
//
FoobarMathToken const* parser_pop( Parser* ctx )
{
	if ( ctx->position >= ctx->tokens_count ) { return NULL; }

	return &ctx->tokens[ctx->position++];
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

//
// Match a token's string against the given identifier string.
//
// This is necessary because the token's content is not a null-terminated string.
//
gboolean parser_match_identifier(
	FoobarMathToken const* token,
	gchar const*           identifier )
{
	if ( strlen( identifier ) != token->length ) { return FALSE; }

	return !strncmp( token->data, identifier, token->length );
}

//
// Get the precedence of an infix operator token or INVALID.
//
InfixPrecedence parser_operator_precedence( FoobarMathToken const* token )
{
	if ( token )
	{
		switch ( token->type )
		{
			case FOOBAR_MATH_TOKEN_PLUS:
			case FOOBAR_MATH_TOKEN_MINUS:
				return INFIX_PRECEDENCE_ADDITIVE;
			case FOOBAR_MATH_TOKEN_MUL:
			case FOOBAR_MATH_TOKEN_DIV:
				return INFIX_PRECEDENCE_MULTIPLICATIVE;
			case FOOBAR_MATH_TOKEN_POW:
				return INFIX_PRECEDENCE_EXPONENTS;
			case FOOBAR_MATH_TOKEN_IDENTIFIER:
			case FOOBAR_MATH_TOKEN_NUMBER:
			case FOOBAR_MATH_TOKEN_PAREN_OPEN:
			case FOOBAR_MATH_TOKEN_PAREN_CLOSE:
			default:
				break;
		}
	}

	return INFIX_PRECEDENCE_INVALID;
}

//
// Get the operation for an infix operator token. This requires the token to be valid.
//
FoobarMathOperation parser_operator( FoobarMathToken const* token )
{
	switch ( token->type )
	{
		case FOOBAR_MATH_TOKEN_PLUS:
			return FOOBAR_MATH_OPERATION_ADD;
		case FOOBAR_MATH_TOKEN_MINUS:
			return FOOBAR_MATH_OPERATION_SUB;
		case FOOBAR_MATH_TOKEN_MUL:
			return FOOBAR_MATH_OPERATION_MUL;
		case FOOBAR_MATH_TOKEN_DIV:
			return FOOBAR_MATH_OPERATION_DIV;
		case FOOBAR_MATH_TOKEN_POW:
			return FOOBAR_MATH_OPERATION_POW;
		case FOOBAR_MATH_TOKEN_IDENTIFIER:
		case FOOBAR_MATH_TOKEN_NUMBER:
		case FOOBAR_MATH_TOKEN_PAREN_OPEN:
		case FOOBAR_MATH_TOKEN_PAREN_CLOSE:
		default:
			g_warn_if_reached( );
			return 0;
	}
}
