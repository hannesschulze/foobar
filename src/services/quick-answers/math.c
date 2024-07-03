#define _GNU_SOURCE

#include "services/quick-answers/math.h"
#include <math.h>

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

FoobarMathExpression* foobar_math_expression_new_value( FoobarMathValue value )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_VALUE;
	res->value.v = value;
	return res;
}

FoobarMathExpression* foobar_math_expression_new_function(
	FoobarMathFunction    function,
    FoobarMathExpression* input )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_FUNCTION;
	res->function.f = function;
	res->function.input = input;
	return res;
}

FoobarMathExpression* foobar_math_expression_new_constant( FoobarMathConstant constant )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_CONSTANT;
	res->constant.c = constant;
	return res;
}

FoobarMathExpression* foobar_math_expression_new_operation(
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
			g_autofree gchar* val = foobar_math_value_to_string( expr->value.v );
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

void foobar_math_value_new_int( FoobarMathValue* out_value )
{
	out_value->type = FOOBAR_MATH_VALUE_INT;
	mpz_init( out_value->int_value.v );
	out_value->int_value.decimal_places = 0;
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

	foobar_math_value_new_int( out_value );
	out_value->int_value.decimal_places = input_length - 1 - sep_pos;
	if ( mpz_init_set_str( out_value->int_value.v, without_sep, 10 ) == -1 )
	{
		mpz_clear( out_value->int_value.v );
		return FALSE;
	}

	return TRUE;
}

void foobar_math_value_from_float(
	long double      value,
	FoobarMathValue* out_value )
{
	out_value->type = FOOBAR_MATH_VALUE_FLOAT;
	out_value->float_value.v = value;
}

void foobar_math_value_copy(
	FoobarMathValue  value,
	FoobarMathValue* out_value )
{
	*out_value = value;

	switch ( value.type )
	{
		case FOOBAR_MATH_VALUE_INT:
			mpz_init_set( out_value->int_value.v, value.int_value.v );
			break;
		case FOOBAR_MATH_VALUE_FLOAT:
			break;
		default:
			g_warn_if_reached( );
			break;
	}
}

void foobar_math_value_free( FoobarMathValue value )
{
	switch ( value.type )
	{
		case FOOBAR_MATH_VALUE_INT:
			mpz_clear( value.int_value.v );
			break;
		case FOOBAR_MATH_VALUE_FLOAT:
			break;
		default:
			g_warn_if_reached( );
			break;
	}
}

void foobar_math_value_negate( FoobarMathValue* value )
{
	switch ( value->type )
	{
		case FOOBAR_MATH_VALUE_INT:
			mpz_neg( value->int_value.v, value->int_value.v );
			break;
		case FOOBAR_MATH_VALUE_FLOAT:
			value->float_value.v = -value->float_value.v;
			break;
		default:
			g_warn_if_reached( );
			break;
	}
}

gboolean foobar_math_value_add(
	FoobarMathValue  lhs,
	FoobarMathValue  rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_is_simple_int( lhs ) && foobar_math_value_is_simple_int( rhs ) )
	{
		foobar_math_value_new_int( out_value );
		mpz_add( out_value->int_value.v, lhs.int_value.v, rhs.int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( lhs );
	long double rhs_float = foobar_math_value_to_float( rhs );
	foobar_math_value_from_float( lhs_float + rhs_float, out_value );
	return TRUE;
}

gboolean foobar_math_value_sub(
	FoobarMathValue  lhs,
	FoobarMathValue  rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_is_simple_int( lhs ) && foobar_math_value_is_simple_int( rhs ) )
	{
		foobar_math_value_new_int( out_value );
		mpz_sub( out_value->int_value.v, lhs.int_value.v, rhs.int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( lhs );
	long double rhs_float = foobar_math_value_to_float( rhs );
	foobar_math_value_from_float( lhs_float - rhs_float, out_value );
	return TRUE;
}

gboolean foobar_math_value_mul(
	FoobarMathValue  lhs,
	FoobarMathValue  rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_is_simple_int( lhs ) && foobar_math_value_is_simple_int( rhs ) )
	{
		foobar_math_value_new_int( out_value );
		mpz_mul( out_value->int_value.v, lhs.int_value.v, rhs.int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( lhs );
	long double rhs_float = foobar_math_value_to_float( rhs );
	foobar_math_value_from_float( lhs_float * rhs_float, out_value );
	return TRUE;
}

gboolean foobar_math_value_div(
	FoobarMathValue  lhs,
	FoobarMathValue  rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_is_simple_int( lhs ) && foobar_math_value_is_simple_int( rhs ) )
	{
		if ( mpz_sgn( rhs.int_value.v ) == 0 ) { return FALSE; }

		foobar_math_value_new_int( out_value );
		mpz_t rem;
		mpz_init( rem );
		mpz_divmod( out_value->int_value.v, rem, lhs.int_value.v, rhs.int_value.v );
		gboolean result_is_int = mpz_sgn( rem ) == 0;
		mpz_clear( rem );

		if ( result_is_int ) { return TRUE; }
		foobar_math_value_free( *out_value );
	}

	long double lhs_float = foobar_math_value_to_float( lhs );
	long double rhs_float = foobar_math_value_to_float( rhs );
	long double result = lhs_float / rhs_float;
	if ( isnanl( result ) || isinfl( result ) ) { return FALSE; }
	foobar_math_value_from_float( result, out_value );
	return TRUE;
}

gboolean foobar_math_value_pow(
	FoobarMathValue  lhs,
	FoobarMathValue  rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_is_simple_int( lhs ) && foobar_math_value_is_simple_int( rhs ) &&
			mpz_fits_ulong_p( rhs.int_value.v ) )
	{
		foobar_math_value_new_int( out_value );
		mpz_pow_ui( out_value->int_value.v, lhs.int_value.v, mpz_get_ui( rhs.int_value.v ) );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( lhs );
	long double rhs_float = foobar_math_value_to_float( rhs );
	long double result = powl( lhs_float, rhs_float );
	if ( isnanl( result ) || isinfl( result ) ) { return FALSE; }
	foobar_math_value_from_float( result, out_value );
	return TRUE;
}

gboolean foobar_math_value_is_simple_int( FoobarMathValue value )
{
	return value.type == FOOBAR_MATH_VALUE_INT && value.int_value.decimal_places == 0;
}

gchar* foobar_math_value_to_string( FoobarMathValue value )
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
				memmove( &result[len + 1 - decimal_places], &result[len - decimal_places], decimal_places + 1 );
				result[len - decimal_places] = '.';
			}
			return result;
		}
		case FOOBAR_MATH_VALUE_FLOAT:
		{
			gchar* result = g_strdup_printf( "%.30Lf", value.float_value.v );

			// trim trailing zeros
			size_t len = strlen( result );
			for ( gsize i = 0; i < len; ++i )
			{
				if ( result[len - 1 - i] == '.' )
				{
					result[len - 1 - i] = '\0';
					break;
				}
				else if ( result[len - 1 - i] == '0' )
				{
					result[len - 1 - i] = '\0';
				}
				else
				{
					break;
				}
			}

			return result;
		}
		default:
			g_warn_if_reached( );
			return NULL;
	}
}

long double foobar_math_value_to_float( FoobarMathValue value )
{
	switch ( value.type )
	{
		case FOOBAR_MATH_VALUE_INT:
		{
			mpz_t exp, integer, frac;
			mpz_inits( exp, integer, frac, NULL );
			mpz_ui_pow_ui( exp, 10, value.int_value.decimal_places );
			mpz_mod( frac, value.int_value.v, exp );
			mpz_div( integer, value.int_value.v, exp );
			mpf_t f_exp, f_frac;
			mpf_inits( f_exp, f_frac, NULL );
			mpf_set_z( f_exp, exp );
			mpf_set_z( f_frac, frac );
			mpf_div( f_frac, f_frac, f_exp );
			long double result = mpz_get_d( integer ) + mpf_get_d( f_frac );
			mpf_clears( f_exp, f_frac, NULL );
			mpz_clears( exp, integer, frac, NULL );
			return result;
		}
		case FOOBAR_MATH_VALUE_FLOAT:
			return value.float_value.v;
		default:
			g_warn_if_reached( );
			return 0;
	}
}

gboolean foobar_math_evaluate(
	FoobarMathExpression const* expr,
	FoobarMathValue*            out_value )
{
	switch ( expr->type )
	{
        case FOOBAR_MATH_EXPRESSION_VALUE:
			foobar_math_value_copy( expr->value.v, out_value );
			return TRUE;
        case FOOBAR_MATH_EXPRESSION_CONSTANT:
			switch ( expr->constant.c )
			{
				case FOOBAR_MATH_CONSTANT_PI:
					foobar_math_value_from_float( M_PIl, out_value );
					return TRUE;
				case FOOBAR_MATH_CONSTANT_E:
					foobar_math_value_from_float( M_El, out_value );
					return TRUE;
				default:
					g_warn_if_reached( );
					return FALSE;
			}
        case FOOBAR_MATH_EXPRESSION_FUNCTION:
		{
			FoobarMathValue input = { 0 };
			if ( !foobar_math_evaluate( expr->function.input, &input ) ) { return FALSE; }

			if ( expr->function.f == FOOBAR_MATH_FUNCTION_NEGATE )
			{
				*out_value = input;
				foobar_math_value_negate( out_value );
				return TRUE;
			}

			long double input_float = foobar_math_value_to_float( input );
			foobar_math_value_free( input );

			long double result = NAN;
			switch ( expr->function.f )
			{
				case FOOBAR_MATH_FUNCTION_SIN:
					result = sinl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_COS:
					result = cosl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_SEC:
					result = 1.L / cosl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_CSC:
					result = 1.L / sinl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_TAN:
					result = tanl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_COT:
					result = 1.L / tanl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCSIN:
					result = asinl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCOS:
					result = acosl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCSEC:
					result = acosl( 1.L / input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCSC:
					result = asinl( 1.L / input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCTAN:
					result = atanl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_ARCCOT:
					result = atanl( 1.L / input_float );
					break;
				case FOOBAR_MATH_FUNCTION_EXP:
					result = expl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_SQRT:
					result = sqrtl( input_float );
					break;
				case FOOBAR_MATH_FUNCTION_NEGATE:
				default:
					g_warn_if_reached( );
					return FALSE;
			}

			if ( isnanl( result ) || isinfl( result ) ) { return FALSE; }

			foobar_math_value_from_float( result, out_value );
			return TRUE;
		}
        case FOOBAR_MATH_EXPRESSION_OPERATION:
		{
			FoobarMathValue lhs = { 0 };
			if ( !foobar_math_evaluate( expr->operation.lhs, &lhs ) ) { return FALSE; }
			FoobarMathValue rhs = { 0 };
			if ( !foobar_math_evaluate( expr->operation.rhs, &rhs ) )
			{
				foobar_math_value_free( lhs );
				return FALSE;
			}

			gboolean success = FALSE;
			switch ( expr->operation.o )
			{
				case FOOBAR_MATH_OPERATION_ADD:
					success = foobar_math_value_add( lhs, rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_SUB:
					success = foobar_math_value_sub( lhs, rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_MUL:
					success = foobar_math_value_mul( lhs, rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_DIV:
					success = foobar_math_value_div( lhs, rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_POW:
					success = foobar_math_value_pow( lhs, rhs, out_value );
					break;
				default:
					g_warn_if_reached( );
					break;
			}

			foobar_math_value_free( lhs );
			foobar_math_value_free( rhs );

			return success;
		}
		default:
			g_warn_if_reached( );
			return FALSE;
	}
}
