#include "services/quick-answers/math.h"

struct _FoobarMathExpression
{
	FoobarMathExpressionType type;

	union
	{
		struct
		{
			FoobarMathValue v;
		} value;

		struct
		{
			FoobarMathFunction f;
			FoobarMathExpression* input;
		} function;

		struct
		{
			FoobarMathConstant c;
		} constant;

		struct
		{
			FoobarMathOperation o;
			FoobarMathExpression* lhs;
			FoobarMathExpression* rhs;
		} operation;
	};
};

FoobarMathExpression* foobar_math_expression_value( FoobarMathValue value )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_VALUE;
	res->value.v = value;
	return res;
}

FoobarMathExpression* foobar_math_expression_function(
	FoobarMathFunction    function,
    FoobarMathExpression* input )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_FUNCTION;
	res->function.f = function;
	res->function.input = input;
	return res;
}

FoobarMathExpression* foobar_math_expression_constant( FoobarMathConstant constant )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_CONSTANT;
	res->constant.c = constant;
	return res;
}

FoobarMathExpression* foobar_math_expression_operation(
	FoobarMathOperation   operation,
	FoobarMathExpression* lhs,
	FoobarMathExpression* rhs )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_OPERATION;
	res->operation.o = operation;
	res->operation.lhs = lhs;
	res->operation.rhs = rhs;
	return res;
}

void foobar_math_expression_free( FoobarMathExpression* expression )
{
	if ( expression )
	{
		switch ( expression->type )
		{
			case FOOBAR_MATH_EXPRESSION_VALUE:
				foobar_math_value_free( expression->value.v );
				break;
			case FOOBAR_MATH_EXPRESSION_FUNCTION:
				foobar_math_expression_free( expression->function.input );
				break;
			case FOOBAR_MATH_EXPRESSION_CONSTANT:
				break;
			case FOOBAR_MATH_EXPRESSION_OPERATION:
				foobar_math_expression_free( expression->operation.lhs );
				foobar_math_expression_free( expression->operation.rhs );
				break;
			default:
				g_warn_if_reached( );
				break;
		}

		g_free( expression );
	}
}

void foobar_math_expression_print(
	FoobarMathExpression const* expr,
	gint                        indentation )
{
	for (gint i = 0; i < indentation; ++i) { g_print("| "); }
	switch ( expr->type )
	{
        case FOOBAR_MATH_EXPRESSION_VALUE:
		{
			g_autofree gchar* val = foobar_math_value_format( expr->value.v );
			g_print( "%s\n", val );
			break;
		}
        case FOOBAR_MATH_EXPRESSION_FUNCTION:
			switch ( expr->function.f )
			{
				case FOOBAR_MATH_FUNCTION_NEGATE:
					g_print( "-\n" );
					break;
				case FOOBAR_MATH_FUNCTION_SIN:
					g_print( "sin\n" );
					break;
				case FOOBAR_MATH_FUNCTION_COS:
					g_print( "cos\n" );
					break;
				case FOOBAR_MATH_FUNCTION_SEC:
					g_print( "sec\n" );
					break;
				case FOOBAR_MATH_FUNCTION_CSC:
					g_print( "csc\n" );
					break;
				case FOOBAR_MATH_FUNCTION_TAN:
					g_print( "tan\n" );
					break;
				case FOOBAR_MATH_FUNCTION_COT:
					g_print( "cot\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCSIN:
					g_print( "arcsin\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCOS:
					g_print( "arccos\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCSEC:
					g_print( "arcsec\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCSC:
					g_print( "arccsc\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCTAN:
					g_print( "arctan\n" );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCOT:
					g_print( "arccot\n" );
					break;
				case FOOBAR_MATH_FUNCTION_EXP:
					g_print( "exp\n" );
					break;
				case FOOBAR_MATH_FUNCTION_SQRT:
					g_print( "sqrt\n" );
					break;
				default:
					g_warn_if_reached( );
					break;
			}
			foobar_math_expression_print( expr->function.input, indentation + 1 );
			break;
        case FOOBAR_MATH_EXPRESSION_CONSTANT:
			switch ( expr->constant.c )
			{
				case FOOBAR_MATH_CONSTANT_PI:
					g_print( "pi\n" );
					break;
				case FOOBAR_MATH_CONSTANT_E:
					g_print( "e\n" );
					break;
				default:
					g_warn_if_reached( );
					break;
			}
			break;
        case FOOBAR_MATH_EXPRESSION_OPERATION:
			switch ( expr->operation.o )
			{
				case FOOBAR_MATH_OPERATION_ADD:
					g_print( "+\n" );
					break;
				case FOOBAR_MATH_OPERATION_SUB:
					g_print( "-\n" );
					break;
				case FOOBAR_MATH_OPERATION_MUL:
					g_print( "*\n" );
					break;
				case FOOBAR_MATH_OPERATION_DIV:
					g_print( "/\n" );
					break;
				case FOOBAR_MATH_OPERATION_POW:
					g_print( "^\n" );
					break;
				default:
					g_warn_if_reached( );
					break;
			}
			foobar_math_expression_print( expr->operation.lhs, indentation + 1 );
			foobar_math_expression_print( expr->operation.rhs, indentation + 1 );
			break;
		default:
			g_warn_if_reached( );
			break;
	}
}

gboolean foobar_math_value_from_string(
	gchar const*     input,
	gsize            input_length,
	FoobarMathValue* out_value )
{
	g_autofree gchar* without_sep = g_new0( gchar, input_length + 1 );
	gsize sep_pos = input_length - 1;
	gsize j = 0;
	for ( gsize i = 0; i < input_length; ++i )
	{
		if ( input[i] == '.' )
		{
			sep_pos = i;
		}
		else
		{
			without_sep[j++] = input[i];
		}
	}

	out_value->type = FOOBAR_MATH_VALUE_INT;
	out_value->int_value.decimal_places = input_length - 1 - sep_pos;
	mpz_init( out_value->int_value.v );
	if ( mpz_init_set_str( out_value->int_value.v, without_sep, 10 ) == -1 )
	{
		mpz_clear( out_value->int_value.v );
		return FALSE;
	}

	return TRUE;
}

void foobar_math_value_free( FoobarMathValue value )
{
	if ( value.type == FOOBAR_MATH_VALUE_INT )
	{
		mpz_clear( value.int_value.v );
	}
}

gboolean foobar_math_evaluate(
	FoobarMathExpression const* expression,
	FoobarMathValue*            out_value )
{
	return FALSE;
}

gchar* foobar_math_value_format( FoobarMathValue value )
{
	switch ( value.type )
	{
		case FOOBAR_MATH_VALUE_INT:
		{
			size_t len = mpz_sizeinbase( value.int_value.v, 10 );
			gchar* result = g_new0( gchar, len + 3 ); // length + null terminator + sign + separator
			mpz_get_str( result, 10, value.int_value.v );
			len = strlen( result );
			gboolean trailing_zeros = TRUE;
			gsize decimal_places = value.int_value.decimal_places;
			gsize actual_decimal_places = decimal_places;
			for ( gsize i = 0; i < decimal_places; ++i )
			{
				if ( trailing_zeros && result[len - 1 - i] == '0' )
				{
					result[len - 1 - i] = '\0';
					actual_decimal_places -= 1;
				}
				else
				{
					trailing_zeros = FALSE;
				}
			}
			if ( actual_decimal_places > 0 )
			{
				memmove( &result[len + 1 - decimal_places], &result[len - decimal_places],decimal_places + 1 );
				result[len - decimal_places] = '.';
			}
			return result;
		}
		case FOOBAR_MATH_VALUE_FLOAT:
			return g_strdup_printf( "%Lf", value.float_value.v );
		default:
			g_warn_if_reached( );
			return NULL;
	}
}
