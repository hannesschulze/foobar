#pragma once

#include <gio/gio.h>
#include <gmp.h>

G_BEGIN_DECLS

typedef enum
{
	FOOBAR_MATH_VALUE_INT,
	FOOBAR_MATH_VALUE_FLOAT,
} FoobarMathValueType;

typedef struct _FoobarMathValue FoobarMathValue;

struct _FoobarMathValue
{
	FoobarMathValueType type;

	union
	{
		struct
		{
			mpz_t v;
			gsize decimal_places;
		} int_value;

		struct
		{
			long double v;
		} float_value;
	};
};

typedef enum
{
	FOOBAR_MATH_TOKEN_IDENTIFIER,
	FOOBAR_MATH_TOKEN_NUMBER,
	FOOBAR_MATH_TOKEN_PAREN_OPEN,
	FOOBAR_MATH_TOKEN_PAREN_CLOSE,
	FOOBAR_MATH_TOKEN_PLUS,
	FOOBAR_MATH_TOKEN_MINUS,
	FOOBAR_MATH_TOKEN_MUL,
	FOOBAR_MATH_TOKEN_DIV,
	FOOBAR_MATH_TOKEN_POW,
} FoobarMathTokenType;

typedef struct _FoobarMathToken FoobarMathToken;

struct _FoobarMathToken
{
	FoobarMathTokenType type;
	gchar const*        data;
	gsize               length;
};

typedef enum
{
	FOOBAR_MATH_EXPRESSION_VALUE,
	FOOBAR_MATH_EXPRESSION_FUNCTION,
	FOOBAR_MATH_EXPRESSION_CONSTANT,
	FOOBAR_MATH_EXPRESSION_OPERATION,
} FoobarMathExpressionType;

typedef enum
{
	FOOBAR_MATH_FUNCTION_NEGATE,
	FOOBAR_MATH_FUNCTION_SIN,
	FOOBAR_MATH_FUNCTION_COS,
	FOOBAR_MATH_FUNCTION_SEC,
	FOOBAR_MATH_FUNCTION_CSC,
	FOOBAR_MATH_FUNCTION_TAN,
	FOOBAR_MATH_FUNCTION_COT,
	FOOBAR_MATH_FUNCTION_ARCSIN,
	FOOBAR_MATH_FUNCTION_ARCCOS,
	FOOBAR_MATH_FUNCTION_ARCSEC,
	FOOBAR_MATH_FUNCTION_ARCCSC,
	FOOBAR_MATH_FUNCTION_ARCTAN,
	FOOBAR_MATH_FUNCTION_ARCCOT,
	FOOBAR_MATH_FUNCTION_EXP,
	FOOBAR_MATH_FUNCTION_SQRT,
} FoobarMathFunction;

typedef enum
{
	FOOBAR_MATH_CONSTANT_PI,
	FOOBAR_MATH_CONSTANT_E,
} FoobarMathConstant;

typedef enum
{
	FOOBAR_MATH_OPERATION_ADD,
	FOOBAR_MATH_OPERATION_SUB,
	FOOBAR_MATH_OPERATION_MUL,
	FOOBAR_MATH_OPERATION_DIV,
	FOOBAR_MATH_OPERATION_POW,
} FoobarMathOperation;

typedef struct _FoobarMathExpression FoobarMathExpression;

FoobarMathToken*      foobar_math_lex                     ( gchar const*                input,
                                                            gsize                       input_length,
                                                            gsize*                      out_count );
FoobarMathExpression* foobar_math_parse                   ( FoobarMathToken const*      tokens,
                                                            gsize                       tokens_count );
FoobarMathExpression* foobar_math_expression_new_value    ( FoobarMathValue             value );
FoobarMathExpression* foobar_math_expression_new_function ( FoobarMathFunction          function,
                                                            FoobarMathExpression*       input );
FoobarMathExpression* foobar_math_expression_new_constant ( FoobarMathConstant          constant );
FoobarMathExpression* foobar_math_expression_new_operation( FoobarMathOperation         operation,
                                                            FoobarMathExpression*       lhs,
                                                            FoobarMathExpression*       rhs );
void                  foobar_math_expression_print        ( FoobarMathExpression const* expr,
                                                            gint                        indentation );
gboolean              foobar_math_evaluate                ( FoobarMathExpression const* expr,
                                                            FoobarMathValue*            out_value );
void                  foobar_math_expression_free         ( FoobarMathExpression*       expression );
void                  foobar_math_value_new_int           ( FoobarMathValue*            out_value );
void                  foobar_math_value_from_float        ( long double                 value,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_from_string       ( gchar const*                input,
                                                            gsize                       input_length,
                                                            FoobarMathValue*            out_value );
void                  foobar_math_value_copy              ( FoobarMathValue             value,
                                                            FoobarMathValue*            out_value );
void                  foobar_math_value_free              ( FoobarMathValue             value );
void                  foobar_math_value_negate            ( FoobarMathValue*            value );
gboolean              foobar_math_value_add               ( FoobarMathValue*            lhs,
                                                            FoobarMathValue*            rhs,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_sub               ( FoobarMathValue*            lhs,
                                                            FoobarMathValue*            rhs,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_mul               ( FoobarMathValue*            lhs,
                                                            FoobarMathValue*            rhs,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_div               ( FoobarMathValue*            lhs,
                                                            FoobarMathValue*            rhs,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_pow               ( FoobarMathValue*            lhs,
                                                            FoobarMathValue*            rhs,
                                                            FoobarMathValue*            out_value );
gboolean              foobar_math_value_unify_integers    ( FoobarMathValue*            a,
                                                            FoobarMathValue*            b );
gchar*                foobar_math_value_to_string         ( FoobarMathValue             value );
long double           foobar_math_value_to_float          ( FoobarMathValue             value );

G_END_DECLS
