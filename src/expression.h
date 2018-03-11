// expression.h
//

#pragma once
#include "ptr.h"

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace algebra {

//------------------------------------------------------------------------------
//! operations and functions
enum class op_type
{
    equality,       //!< `lhs` = `rhs`
    sum,            //!< `lhs` + `rhs`
    difference,     //!< `lhs` - `rhs`
    negative,       //!< 0 - `lhs`
    product,        //!< `lhs` * `rhs`
    quotient,       //!< `lhs` / `rhs`
    reciprocal,     //!< 1 / `lhs`
    exponent,       //!< `lhs` raised to the power of `rhs`
    logarithm,      //!< logarithm of `lhs` using base `rhs`
    sine,           //!< sine of `lhs`
    cosine,         //!< cosine of  `lhs`
    tangent,        //!< tangent of `lhs`
    secant,         //!< secant of `lhs`
    cosecant,       //!< cosecant of `lhs`
    cotangent,      //!< cotangent of `lhs`
    arcsine,        //!< inverse sine of `lhs`
    arccosine,      //!< inverse cosine of `lhs`
    arctangent,     //!< inverse tangent of `lhs`
    arcsecant,      //!< inverse secant of `lhs`
    arccosecant,    //!< inverse cosecant of `lhs`
    arccotangent,   //!< inverse cotangent of `lhs`
    derivative,     //!< derivative of `lhs` with respect to `rhs`
    integral,       //!< integral of `lhs` with respect to `rhs`
    differential,   //!< differential of `lhs` for integration
};

class expression;

//------------------------------------------------------------------------------
//! common constant and transcendental values
enum class constant
{
    undefined,  //!< e.g. divide by zero
    pi,         //!<
    e,          //!< natural base
    i,          //!< imaginary unit
};

//------------------------------------------------------------------------------
//! operator
struct op
{
    op_type type;
    ptr<expression> lhs;
    ptr<expression> rhs;
};

//------------------------------------------------------------------------------
//! placeholder value for pattern matching and substitution
enum class placeholder
{
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
};

//------------------------------------------------------------------------------
//! empty expression for unused operands
struct empty {};

//------------------------------------------------------------------------------
//! scalar value
using value = double;

//------------------------------------------------------------------------------
//! variable/symbol
using symbol = std::string;

//------------------------------------------------------------------------------
using expression_base = std::variant<empty, op, constant, value, symbol, placeholder>;
class expression : public expression_base
{
public:
    using expression_base::expression_base;
};

//------------------------------------------------------------------------------
//! transformation pattern for simplifying expressions
struct transform
{
    expression source;
    expression target;
};

//------------------------------------------------------------------------------
expression simplify(expression const& expr, std::size_t max_operations = SIZE_MAX, std::size_t max_iterations = SIZE_MAX);

} // namespace algebra
