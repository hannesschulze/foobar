#include "services/quick-answers/math.h"
#include <mutest.h>

#define SERIALIZATION_TEST( value, identifier )                                                  \
	static void serialization_value_##identifier##_spec( void )                                  \
	{                                                                                            \
		FoobarMathValue val;                                                                     \
                                                                                                 \
		gchar const input[] = #value;                                                            \
		mutest_expect(                                                                           \
			"parse result",                                                                      \
			mutest_bool_value( foobar_math_value_from_string( input, sizeof(input) - 1, &val) ), \
			mutest_to_be_true,                                                                   \
			NULL );                                                                              \
		g_autofree gchar* output = foobar_math_value_to_string( val );                           \
		mutest_expect(                                                                           \
			"string representation",                                                             \
			mutest_string_value( output ),                                                       \
			mutest_to_be,                                                                        \
			input,                                                                               \
			NULL );                                                                              \
                                                                                                 \
		foobar_math_value_free( val );                                                           \
	}

SERIALIZATION_TEST( 0, 0 )
SERIALIZATION_TEST( 1, 1 )
SERIALIZATION_TEST( -1, neg_1 )
SERIALIZATION_TEST( 1.2, 1_2 )
SERIALIZATION_TEST( -1.2, neg_1_2 )
SERIALIZATION_TEST( 1.30002, 1_30002 )
SERIALIZATION_TEST( -1.30002, neg_1_30002 )
SERIALIZATION_TEST( 0.2, 0_2 )
SERIALIZATION_TEST( -0.2, neg_0_2 )
SERIALIZATION_TEST( 0.00002, 0_00002 )
SERIALIZATION_TEST( -0.00002, neg_0_00002 )
SERIALIZATION_TEST( 123, 123 )
SERIALIZATION_TEST( 123.456, 123_456 )
SERIALIZATION_TEST( -123, neg_123 )
SERIALIZATION_TEST( -123.456, neg_123_456 )

#undef SERIALIZATION_TEST

static void serialization_suite( void )
{
	mutest_it( "value 0", serialization_value_0_spec );
	mutest_it( "value 1", serialization_value_1_spec );
	mutest_it( "value -1", serialization_value_neg_1_spec );
	mutest_it( "value 1.2", serialization_value_1_2_spec );
	mutest_it( "value -1.2", serialization_value_neg_1_2_spec );
	mutest_it( "value 1.30002", serialization_value_1_30002_spec );
	mutest_it( "value -1.30002", serialization_value_neg_1_30002_spec );
	mutest_it( "value 0.2", serialization_value_0_2_spec );
	mutest_it( "value -0.2", serialization_value_neg_0_2_spec );
	mutest_it( "value 0.00002", serialization_value_0_00002_spec );
	mutest_it( "value -0.00002", serialization_value_neg_0_00002_spec );
	mutest_it( "value 123", serialization_value_123_spec );
	mutest_it( "value 123.456", serialization_value_123_456_spec );
	mutest_it( "value -123", serialization_value_neg_123_spec );
	mutest_it( "value -123.456", serialization_value_neg_123_456_spec );
}
#define OPERATION_TEST( op, value_a, value_b, value_res, identifier )                              \
	static void operation_##identifier##_spec( void )                                              \
	{                                                                                              \
		FoobarMathValue a, b, res;                                                                 \
                                                                                                   \
		gchar const input_a[] = #value_a;                                                          \
		gchar const input_b[] = #value_b;                                                          \
		mutest_expect(                                                                             \
			"input a parsed",                                                                      \
			mutest_bool_value( foobar_math_value_from_string( input_a, sizeof(input_a) - 1, &a) ), \
			mutest_to_be_true,                                                                     \
			NULL );                                                                                \
		mutest_expect(                                                                             \
			"input b parsed",                                                                      \
			mutest_bool_value( foobar_math_value_from_string( input_b, sizeof(input_b) - 1, &b) ), \
			mutest_to_be_true,                                                                     \
			NULL );                                                                                \
		foobar_math_value_##op( &a, &b, &res );                                                    \
		g_autofree gchar* output = foobar_math_value_to_string( res );                             \
		mutest_expect(                                                                             \
			"result",                                                                              \
			mutest_string_value( output ),                                                         \
			mutest_to_be,                                                                          \
			#value_res,                                                                            \
			NULL );                                                                                \
                                                                                                   \
		foobar_math_value_free( a );                                                               \
		foobar_math_value_free( b );                                                               \
		foobar_math_value_free( res );                                                             \
	}

OPERATION_TEST( add, 123, 456, 579, 123_plus_456)
OPERATION_TEST( add, 123.456, 11, 134.456, 123_456_plus_11)
OPERATION_TEST( add, 11, 123.456, 134.456, 11_plus_123_456)
OPERATION_TEST( sub, 123, 456, -333, 123_minus_456)
OPERATION_TEST( sub, 123.456, 11, 112.456, 123_456_minus_11)
OPERATION_TEST( sub, 11, 123.456, -112.456, 11_minus_123_456)
OPERATION_TEST( mul, 123, 3, 369, 123_times_3 )
OPERATION_TEST( mul, 12, 0.05, 0.6, 12_times_0_05 )
OPERATION_TEST( mul, 0.05, 12, 0.6, 0_05_times_12 )
OPERATION_TEST( div, 369, 3, 123, 369_div_3 )
OPERATION_TEST( div, 369, 2, 184.5, 369_div_2 )
OPERATION_TEST( div, 12, 0.05, 240, 12_div_0_05 )
OPERATION_TEST( div, 0.08, 4, 0.02, 0_08_div_4 )
OPERATION_TEST( pow, 2, 3, 8, 2_pow_3 )
OPERATION_TEST( pow, 4, 0.5, 2, 4_pow_0_5 )

#undef OPERATION_TEST

static void addition_suite( void )
{
	mutest_it( "calculate 123 + 456", operation_123_plus_456_spec );
	mutest_it( "calculate 123.456 + 11", operation_123_456_plus_11_spec );
	mutest_it( "calculate 11 + 123.456", operation_11_plus_123_456_spec );
}

static void subtraction_suite( void )
{
	mutest_it( "calculate 123 - 456", operation_123_minus_456_spec );
	mutest_it( "calculate 123.456 - 11", operation_123_456_minus_11_spec );
	mutest_it( "calculate 11 - 123.456", operation_11_minus_123_456_spec );
}

static void multiplication_suite( void )
{
	mutest_it( "calculate 123 * 3", operation_123_times_3_spec );
	mutest_it( "calculate 12 * 0.05", operation_12_times_0_05_spec );
	mutest_it( "calculate 0.05 * 12", operation_0_05_times_12_spec );
}

static void division_suite( void )
{
	mutest_it( "calculate 369 / 3", operation_369_div_3_spec );
	mutest_it( "calculate 369 / 2", operation_369_div_2_spec );
	mutest_it( "calculate 12 / 0.05", operation_12_div_0_05_spec );
	mutest_it( "calculate 0.08 / 4", operation_0_08_div_4_spec );
}

static void power_suite( void )
{
	mutest_it( "calculate 2 ^ 3", operation_2_pow_3_spec );
	mutest_it( "calculate 4 ^ 0.5", operation_4_pow_0_5_spec );
}

MUTEST_MAIN(
	mutest_describe( "Serialization", serialization_suite );
	mutest_describe( "Addition", addition_suite );
	mutest_describe( "Subtraction", subtraction_suite );
	mutest_describe( "Multiplication", multiplication_suite );
	mutest_describe( "Division", division_suite );
	mutest_describe( "Power", power_suite );
)
