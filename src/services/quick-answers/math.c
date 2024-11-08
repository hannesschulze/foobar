#define _GNU_SOURCE

#include "services/quick-answers/math.h"
#include <math.h>

//
// FoobarMathExpression:
//
// A fully parsed mathematical expression which can be evaluated, for example "sin(pi) - 3" would be represented as:
//  - Operation: SUB
//    - Function: SIN
//      - Constant: PI
//    - Value: 3
//

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

//
// FoobarMathValue:
//
// Representation of a value. The goal of this memory structure is to store the value as an integer for as long as
// possible. Because of this, there is a "decimal_places" field, so the actual value of an integer is
// $v * 10^(-decimal_places)$.
//
// If absolutely necessary (for example, because we use a function like sin), the value is converted to a floating-point
// value.
//
// Note that we use GMP to store the integer value to allow for arbitrarily large values. Because of this, the structure
// should always be freed using foobar_math_value_free.
//

// ---------------------------------------------------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------------------------------------------------

//
// Helper to allocate a new FoobarMathExpression representing a value.
//
// Ownership of "value" is transferred to the new expression.
//
FoobarMathExpression* foobar_math_expression_new_value( FoobarMathValue value )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_VALUE;
	res->value.v = value;
	return res;
}

//
// Helper to allocate a new FoobarMathExpression representing a function to be evaluated with another expression as its
// parameter.
//
// Ownership of "input" is transferred to the new expression.
//
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

//
// Helper to allocate a new FoobarMathExpression representing a known constant.
//
FoobarMathExpression* foobar_math_expression_new_constant( FoobarMathConstant constant )
{
	FoobarMathExpression* res = g_new0( FoobarMathExpression, 1 );
	res->type = FOOBAR_MATH_EXPRESSION_CONSTANT;
	res->constant.c = constant;
	return res;
}

//
// Helper to allocate a new FoobarMathExpression representing an operation to be evaluated with two expressions as its
// parameters.
//
// Ownership of "lhs" and "rhs" is transferred to the new expression.
//
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

//
// Release the memory associated with a mathematical expression, along with the expressions it owns.
//
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

//
// Print a tree representation of a mathematical expression.
//
// This is mainly useful for debugging. "indentation" describes the current indentation level for the recursive call
// (should be 0 initially).
//
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

//
// Evaluate an expression, producing a value.
//
// On success (indicated by the return value TRUE), the value should be freed using foobar_math_value_free.
//
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
					success = foobar_math_value_add( &lhs, &rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_SUB:
					success = foobar_math_value_sub( &lhs, &rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_MUL:
					success = foobar_math_value_mul( &lhs, &rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_DIV:
					success = foobar_math_value_div( &lhs, &rhs, out_value );
					break;
				case FOOBAR_MATH_OPERATION_POW:
					success = foobar_math_value_pow( &lhs, &rhs, out_value );
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

// ---------------------------------------------------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------------------------------------------------

//
// Initialize a new integer value to zero.
//
// After this, the structure should be freed using foobar_math_value_free.
//
void foobar_math_value_new_int( FoobarMathValue* out_value )
{
	out_value->type = FOOBAR_MATH_VALUE_INT;
	mpz_init( out_value->int_value.v );
	out_value->int_value.decimal_places = 0;
}

//
// Initialize a new value from its floating point representation.
//
// Although technically not necessary, you should still call foobar_math_value_free on the returned value.
//
void foobar_math_value_from_float(
	long double      value,
	FoobarMathValue* out_value )
{
	out_value->type = FOOBAR_MATH_VALUE_FLOAT;
	out_value->float_value.v = value;
}

//
// Parse a value from its string representation. This will always return an integer representation.
//
// On success (return value TRUE), the structure should be freed using foobar_math_value_free.
//
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

//
// Create a copy of a value, allocating necessary memory.
//
// After this, the structure should be freed using foobar_math_value_free.
//
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

//
// Release memory associated with a value.
//
// This is necessary because the integer representation is stored using GMP, which can store arbitrarily large values.
//
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

// ---------------------------------------------------------------------------------------------------------------------
// Operations
// ---------------------------------------------------------------------------------------------------------------------

//
// Operation equivalent to "value *= -1".
//
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

//
// Operation setting "out_value = lhs + rhs".
//
// After this, the result value should be freed using foobar_math_value_free.
//
gboolean foobar_math_value_add(
	FoobarMathValue* lhs,
	FoobarMathValue* rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_unify_integers( lhs, rhs ) )
	{
		foobar_math_value_new_int( out_value );
		out_value->int_value.decimal_places = lhs->int_value.decimal_places;
		mpz_add( out_value->int_value.v, lhs->int_value.v, rhs->int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( *lhs );
	long double rhs_float = foobar_math_value_to_float( *rhs );
	foobar_math_value_from_float( lhs_float + rhs_float, out_value );
	return TRUE;
}

//
// Operation setting "out_value = lhs - rhs".
//
// After this, the result value should be freed using foobar_math_value_free.
//
gboolean foobar_math_value_sub(
	FoobarMathValue* lhs,
	FoobarMathValue* rhs,
	FoobarMathValue* out_value )
{
	if ( foobar_math_value_unify_integers( lhs, rhs ) )
	{
		foobar_math_value_new_int( out_value );
		out_value->int_value.decimal_places = lhs->int_value.decimal_places;
		mpz_sub( out_value->int_value.v, lhs->int_value.v, rhs->int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( *lhs );
	long double rhs_float = foobar_math_value_to_float( *rhs );
	foobar_math_value_from_float( lhs_float - rhs_float, out_value );
	return TRUE;
}

//
// Operation setting "out_value = lhs * rhs".
//
// After this, the result value should be freed using foobar_math_value_free.
//
gboolean foobar_math_value_mul(
	FoobarMathValue* lhs,
	FoobarMathValue* rhs,
	FoobarMathValue* out_value )
{
	if ( lhs->type == FOOBAR_MATH_VALUE_INT && rhs->type == FOOBAR_MATH_VALUE_INT )
	{
		foobar_math_value_new_int( out_value );
		out_value->int_value.decimal_places = lhs->int_value.decimal_places + rhs->int_value.decimal_places;
		mpz_mul( out_value->int_value.v, lhs->int_value.v, rhs->int_value.v );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( *lhs );
	long double rhs_float = foobar_math_value_to_float( *rhs );
	foobar_math_value_from_float( lhs_float * rhs_float, out_value );
	return TRUE;
}

//
// Operation setting "out_value = lhs / rhs".
//
// After this, the result value should be freed using foobar_math_value_free.
//
gboolean foobar_math_value_div(
	FoobarMathValue* lhs,
	FoobarMathValue* rhs,
	FoobarMathValue* out_value )
{
	if ( lhs->type == FOOBAR_MATH_VALUE_INT && rhs->type == FOOBAR_MATH_VALUE_INT )
	{
		if ( mpz_sgn( rhs->int_value.v ) == 0 ) { return FALSE; }

		if ( rhs->int_value.decimal_places > lhs->int_value.decimal_places )
		{
			foobar_math_value_unify_integers( lhs, rhs );
		}

		foobar_math_value_new_int( out_value );
		out_value->int_value.decimal_places = lhs->int_value.decimal_places - rhs->int_value.decimal_places;
		mpz_t rem;
		mpz_init( rem );
		mpz_divmod( out_value->int_value.v, rem, lhs->int_value.v, rhs->int_value.v );
		gboolean result_is_int = mpz_sgn( rem ) == 0;
		mpz_clear( rem );

		if ( result_is_int ) { return TRUE; }
		foobar_math_value_free( *out_value );
	}

	long double lhs_float = foobar_math_value_to_float( *lhs );
	long double rhs_float = foobar_math_value_to_float( *rhs );
	long double result = lhs_float / rhs_float;
	if ( isnanl( result ) || isinfl( result ) ) { return FALSE; }
	foobar_math_value_from_float( result, out_value );
	return TRUE;
}

//
// Operation setting "out_value = lhs ^ rhs".
//
// After this, the result value should be freed using foobar_math_value_free.
//
gboolean foobar_math_value_pow(
	FoobarMathValue* lhs,
	FoobarMathValue* rhs,
	FoobarMathValue* out_value )
{
	if ( lhs->type == FOOBAR_MATH_VALUE_INT &&
			rhs->type == FOOBAR_MATH_VALUE_INT &&
			rhs->int_value.decimal_places == 0 &&
			mpz_fits_ulong_p( rhs->int_value.v ) )
	{
		unsigned long exp = mpz_get_ui( rhs->int_value.v );
		foobar_math_value_new_int( out_value );
		out_value->int_value.decimal_places = lhs->int_value.decimal_places * exp;
		mpz_pow_ui( out_value->int_value.v, lhs->int_value.v, exp );
		return TRUE;
	}

	long double lhs_float = foobar_math_value_to_float( *lhs );
	long double rhs_float = foobar_math_value_to_float( *rhs );
	long double result = powl( lhs_float, rhs_float );
	if ( isnanl( result ) || isinfl( result ) ) { return FALSE; }
	foobar_math_value_from_float( result, out_value );
	return TRUE;
}

//
// Ensure that two integer values have the same number of decimal places.
//
// This may change the in-memory-representation of a value, but not its actual value. It only increases the precision of
// one value
//
// If either a or b is not stored as an integer, this will return FALSE.
//
gboolean foobar_math_value_unify_integers(
	FoobarMathValue* a,
	FoobarMathValue* b )
{
	if ( a->type != FOOBAR_MATH_VALUE_INT || b->type != FOOBAR_MATH_VALUE_INT ) { return FALSE; }

	// Extend the less precise representation so both numbers have the same number of decimal places.

	gsize decimal_places = MAX( a->int_value.decimal_places, b->int_value.decimal_places );

	mpz_t exp;
	mpz_init( exp );

	if ( decimal_places > a->int_value.decimal_places )
	{
		mpz_ui_pow_ui( exp, 10, decimal_places - a->int_value.decimal_places );
		mpz_mul( a->int_value.v, a->int_value.v, exp );
	}

	if ( decimal_places > b->int_value.decimal_places )
	{
		mpz_ui_pow_ui( exp, 10, decimal_places - b->int_value.decimal_places );
		mpz_mul( b->int_value.v, b->int_value.v, exp );
	}

	mpz_clear( exp );

	a->int_value.decimal_places = b->int_value.decimal_places = decimal_places;

	return TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Conversions
// ---------------------------------------------------------------------------------------------------------------------

//
// Format a value as a string.
//
// The result is a null-terminated string which should be freed using g_free.
//
gchar* foobar_math_value_to_string( FoobarMathValue value )
{
	switch ( value.type )
	{
		case FOOBAR_MATH_VALUE_INT:
		{
			gsize cap = mpz_sizeinbase( value.int_value.v, 10 );
			cap = MAX( cap, value.int_value.decimal_places + 1 ); // pad with zeroes if necessary
			gchar* result = g_new0( gchar, cap + 3 ); // length + null terminator + sign + separator
			mpz_get_str( result, 10, value.int_value.v );
			gsize len = strlen( result ); // actual size may be 1 char less (+ len now also includes sign)
			gboolean has_sign = result[0] == '-';
			if ( has_sign )
			{
				// add the sign back later, makes everything else easier
				result += 1;
				len -= 1;
			}
			// pad with leading zeroes if necessary
			gsize padded_len = MAX( len, value.int_value.decimal_places + 1 );
			memmove( &result[padded_len - len], result, len + 1 );
			memset( result, '0', padded_len - len );
			len = padded_len;
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
			if ( has_sign )
			{
				result -= 1;
				len += 1;
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

//
// Convert a value to its floating-point representation.
//
// This may be a lossy conversion and should only be done if absolutely necessary.
//
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
