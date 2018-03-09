// expression.cpp
//

#include "expression.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include <cstdio>
#include <iostream>

transform transforms[] = {
    // associativity of addition
    //      (x + y) + z  <=>  x + (y + z)
    { op{op_type::sum, op{op_type::sum, placeholder::x, placeholder::y}, placeholder::z}, op{op_type::sum, placeholder::x, op{op_type::sum, placeholder::y, placeholder::z}} },

    // associativity of multiplication
    //      (x + y) + z  <=>  x + (y + z)
    { op{op_type::product, op{op_type::product, placeholder::x, placeholder::y}, placeholder::z}, op{op_type::product, placeholder::x, op{op_type::product, placeholder::y, placeholder::z}} },

    // commutativity of addition
    //      x + y  <=>  y + x
    { op{op_type::sum, placeholder::x, placeholder::y}, op{op_type::sum, placeholder::y, placeholder::x} },

    // commutativity of multiplication
    //      x * y  <=>  y * x
    { op{op_type::product, placeholder::x, placeholder::y},  op{op_type::product, placeholder::y, placeholder::x} },

    // distributivity of multiplication over addition
    //      a * (x + y)  <=>  a * x + a * y
    { op{op_type::product, placeholder::a, op{op_type::sum, placeholder::x, placeholder::y}}, op{op_type::sum, op{op_type::product, placeholder::a, placeholder::x}, op{op_type::product, placeholder::a, placeholder::y}} },

    // additive identity
    //      x + 0  <=>  x
    { op{op_type::sum, placeholder::x, constant::zero}, placeholder::x },

    // multiplicative identity
    //      x * 1  <=>  x
    { op{op_type::product, placeholder::x, constant::one}, placeholder::x },

    // multiplicative kernel
    //      x * 0  <=>  0
    { op{op_type::product, placeholder::x, constant::zero}, constant::zero },

    // additive inverse
    //      x + (-x)  <=>  0
    { op{op_type::difference, placeholder::x, placeholder::x}, constant::zero },

    // multiplicative inverse
    //      x * (x^-1)  <=>  1
    { op{op_type::quotient, placeholder::x, placeholder::x}, constant::one },

    // conversion between constants and from constant to value
    //      note: no conversions from transcendentals to values
    { constant::zero, value{0.0} },
    { constant::one, value{1.0} },
    { constant::two, value{2.0} },
    { constant::two, op{op_type::sum, constant::one, constant::one} },
    { constant::twopi, op{op_type::product, constant::two, constant::pi} },
    { constant::halfpi, op{op_type::quotient, constant::pi, constant::two} },

    //      x + x  <=>  x * 2
    { op{op_type::sum, placeholder::x, placeholder::x}, op{op_type::product, placeholder::x, constant::two} },
    //      x * x  <=>  x ^ 2
    { op{op_type::product, placeholder::x, placeholder::x}, op{op_type::exponent, placeholder::x, constant::two} },


    //
    //  exponentiation and logarithms
    //

    //      log(x * y, b)  <=>  log(x, b) + log(y, b)
    { op{op_type::logarithm, op{op_type::product, placeholder::x, placeholder::y}}, op{op_type::sum, op{op_type::logarithm, placeholder::x, placeholder::b}, op{op_type::logarithm, placeholder::y, placeholder::b}} },

    // change of base
    //      log(x, b)  <=>  log(x, y) / log(b, y)
    { op{op_type::logarithm, placeholder::x, placeholder::b}, op{op_type::quotient, op{op_type::logarithm, placeholder::x, placeholder::y},
                                                                                    op{op_type::logarithm, placeholder::b, placeholder::y}} },

    //      b ^ log(x, b)  <=>  x
    { op{op_type::exponent, placeholder::b, op{op_type::logarithm, placeholder::x, placeholder::b}}, placeholder::x },

    // exponentiation identity
    //      b ^ x * b ^ y  <=>  b ^ (x + y)
    { op{op_type::product, op{op_type::exponent, placeholder::b, placeholder::x}, op{op_type::exponent, placeholder::b, placeholder::y}}, op{op_type::exponent, placeholder::b, op{op_type::sum, placeholder::x, placeholder::y}} },

    //      (b ^ x) ^ y  <=>  b ^ (x * y)
    { op{op_type::exponent, op{op_type::exponent, placeholder::b, placeholder::x}, placeholder::y}, op{op_type::exponent, placeholder::b, op{op_type::product, placeholder::x, placeholder::y}} },

    // distributivity over multiplication
    //      (x * y) ^ n  <=>  (x ^ n) * (y ^ n)
    { op{op_type::exponent, op{op_type::product, placeholder::x, placeholder::y}, placeholder::n}, op{op_type::product, op{op_type::exponent, placeholder::x, placeholder::n},
                                                                                                                        op{op_type::exponent, placeholder::y, placeholder::n}} },

    //      x ^ 0  <=>  1
    { op{op_type::exponent, placeholder::x, constant::zero}, constant::one },

    //      x ^ 1  <=>  x
    { op{op_type::exponent, placeholder::x, constant::one}, placeholder::x },

    //      log(1, x)  <=>  0
    { op{op_type::logarithm, constant::one, placeholder::x}, constant::zero },

    //
    //  complex numbers
    //

    // fundamental property of i
    //      i²  <=>  -1
    { op{op_type::product, constant::i, constant::i}, op{op_type::difference, constant::zero, constant::one} },
    // euler's formula
    //      e ^ (i * x) = cos(x) + i * sin(x)
    { op{op_type::exponent, constant::e, op{op_type::product, constant::i, placeholder::x}}, op{op_type::sum, op{op_type::cosine, placeholder::x},
                                                                                                              op{op_type::product, constant::i, op{op_type::sine, placeholder::x}}} },

    //
    //  trigonometry
    //

    //      sin(0)  <=>  0
    { op{op_type::sine, constant::zero}, constant::zero },
    //      cos(0)  <=>  1
    { op{op_type::cosine, constant::zero}, constant::one },
    //      sin(pi/2)  <=>  1
    { op{op_type::sine, constant::halfpi}, constant::one },
    //      cos(pi/2)  <=>  0
    { op{op_type::cosine, constant::halfpi}, constant::zero },

    //      tan(x)  <=>  sin(x) / cos(x)
    { op{op_type::tangent, placeholder::x}, op{op_type::quotient, op{op_type::sine, placeholder::x}, op{op_type::cosine, placeholder::x}} },
    //      sec(x)  <=>  1 / cos(x)
    { op{op_type::secant, placeholder::x}, op{op_type::quotient, constant::one, op{op_type::cosine, placeholder::x}} },
    //      csc(x)  <=>  1 / sin(x)
    { op{op_type::cosecant, placeholder::x}, op{op_type::quotient, constant::one, op{op_type::sine, placeholder::x}} },
    //      cot(x)  <=>  1 / tan(x)
    { op{op_type::cotangent, placeholder::x}, op{op_type::quotient, constant::one, op{op_type::tangent, placeholder::x}} },
    //      1  <=>  sin²(x) + cos²(x)
    { constant::one, op{op_type::sum, op{op_type::exponent, op{op_type::sine, placeholder::x}, constant::two}, op{op_type::exponent, op{op_type::cosine, placeholder::x}, constant::two}} },

    //      sin(-x) = -sin(x)
    { op{op_type::sine, op{op_type::difference, constant::zero, placeholder::x}}, op{op_type::difference, constant::zero, op{op_type::sine, placeholder::x}} },
    //      cos(-x) = cos(x)
    { op{op_type::cosine, op{op_type::difference, constant::zero, placeholder::x}}, op{op_type::sine, placeholder::x} },
    //      tan(-x) = -tan(x)
    { op{op_type::tangent, op{op_type::difference, constant::zero, placeholder::x}}, op{op_type::difference, constant::zero, op{op_type::tangent, placeholder::x}} },

    //      sin(pi/2 - x)  <=>  cos(x)
    { op{op_type::sine, op{op_type::difference, constant::halfpi, placeholder::x}}, op{op_type::cosine, placeholder::x} },
    //      cos(pi/2 - x)  <=>  sin(x)
    { op{op_type::cosine, op{op_type::difference, constant::halfpi, placeholder::x}}, op{op_type::cosine, placeholder::x} },
    //      tan(pi/2 - x)  <=>  cot(x)
    { op{op_type::tangent, op{op_type::difference, constant::halfpi, placeholder::x}}, op{op_type::cotangent, placeholder::x} },

    //      sin(pi - x)  <=>  sin(x)
    { op{op_type::sine, op{op_type::difference, constant::pi, placeholder::x}}, op{op_type::sine, placeholder::x} },
    //      cos(pi - x)  <=>  -cos(x)
    { op{op_type::cosine, op{op_type::difference, constant::pi, placeholder::x}}, op{op_type::difference, constant::zero, op{op_type::cosine, placeholder::x}} },
    //      tan(pi - x)  <=>  -tan(x)
    { op{op_type::tangent, op{op_type::difference, constant::pi, placeholder::x}}, op{op_type::difference, constant::zero, op{op_type::tangent, placeholder::x}} },

    //      sin(2pi - x)  <=>  sin(-x)
    { op{op_type::sine, op{op_type::difference, constant::twopi, placeholder::x}}, op{op_type::sine, op{op_type::difference, constant::zero, placeholder::x}} },
    //      cos(2pi - x)  <=>  cos(-x)
    { op{op_type::cosine, op{op_type::difference, constant::twopi, placeholder::x}}, op{op_type::cosine, op{op_type::difference, constant::zero, placeholder::x}} },
    //      tan(2pi - x)  <=>  tan(-x)
    { op{op_type::tangent, op{op_type::difference, constant::twopi, placeholder::x}}, op{op_type::tangent, op{op_type::difference, constant::zero, placeholder::x}} },

    //      sin(x + y)  <=>  sin(x) * cos(y) + cos(x) * sin(y)
    { op{op_type::sine, op{op_type::sum, placeholder::x, placeholder::y}}, op{op_type::sum, op{op_type::product, op{op_type::sine, placeholder::x},
                                                                                                                 op{op_type::cosine, placeholder::y}},
                                                                                            op{op_type::product, op{op_type::cosine, placeholder::x},
                                                                                                                 op{op_type::sine, placeholder::y}}} },
    //      sin(x - y)  <=>  sin(x) * cos(y) - cos(x) * sin(y)
    { op{op_type::sine, op{op_type::difference, placeholder::x, placeholder::y}}, op{op_type::difference, op{op_type::product, op{op_type::sine, placeholder::x},
                                                                                                                               op{op_type::cosine, placeholder::y}},
                                                                                                          op{op_type::product, op{op_type::cosine, placeholder::x},
                                                                                                                               op{op_type::sine, placeholder::y}}} },

    //      cos(x + y)  <=>  cos(x) * cos(y) - sin(x) * sin(y)
    { op{op_type::cosine, op{op_type::sum, placeholder::x, placeholder::y}}, op{op_type::difference, op{op_type::product, op{op_type::cosine, placeholder::x},
                                                                                                                          op{op_type::cosine, placeholder::y}},
                                                                                                     op{op_type::product, op{op_type::sine, placeholder::x},
                                                                                                                          op{op_type::sine, placeholder::y}}} },
    //      cos(x - y)  <=>  cos(x) * cos(y) + sin(x) * sin(y)
    { op{op_type::cosine, op{op_type::difference, placeholder::x, placeholder::y}}, op{op_type::sum, op{op_type::product, op{op_type::cosine, placeholder::x},
                                                                                                                          op{op_type::cosine, placeholder::y}},
                                                                                                     op{op_type::product, op{op_type::sine, placeholder::x},
                                                                                                                          op{op_type::sine, placeholder::y}}} },

    //      sin(2x)  <=>  2 * sin(x) * cos(x)
    //      cos(2x)  <=>  cos²(x) - sin²(x)
    //      cos(2x)  <=>  2 * cos²(x) - 1

    //      sin(3x)  <=>  3 * sin(x) - 4 * sin³(x)
    //      cos(3x)  <=>  4 * cos³(x) - 3 * cos(x)

    //      sin²(x)  <=>  (1 - cos(2x)) / 2
    //      cos²(x)  <=>  (1 + cos(2x)) / 2
};

//------------------------------------------------------------------------------
std::string to_string(expression const& in)
{
    if (std::holds_alternative<op>(in)) {
        op const& in_op = std::get<op>(in);
        switch (in_op.type) {
            case op_type::sum: return std::string("(") + to_string(in_op.lhs) + " + " + to_string(in_op.rhs) + ")";
            case op_type::difference: return std::string("(") + to_string(in_op.lhs) + " - " + to_string(in_op.rhs) + ")";
            case op_type::product: return std::string("(") + to_string(in_op.lhs) + " * " + to_string(in_op.rhs) + ")";
            case op_type::quotient: return std::string("(") + to_string(in_op.lhs) + " / " + to_string(in_op.rhs) + ")";
            case op_type::exponent: return std::string("(") + to_string(in_op.lhs) + " ^ " + to_string(in_op.rhs) + ")";
            case op_type::logarithm: return std::string("log(") + to_string(in_op.lhs) + ", " + to_string(in_op.rhs) + ")";
            case op_type::sine: return std::string("sin(") + to_string(in_op.lhs) + ")";
            case op_type::cosine: return std::string("cos(") + to_string(in_op.lhs) + ")";
            case op_type::tangent: return std::string("tan(") + to_string(in_op.lhs) + ")";
            case op_type::secant: return std::string("sec(") + to_string(in_op.lhs) + ")";
            case op_type::cosecant: return std::string("csc(") + to_string(in_op.lhs) + ")";
            case op_type::cotangent: return std::string("cot(") + to_string(in_op.lhs) + ")";
            default: assert(0); return "";
        }
    } else if (std::holds_alternative<value>(in)) {
        return std::to_string(std::get<value>(in));
    } else if (std::holds_alternative<constant>(in)) {
        constant const& in_const = std::get<constant>(in);
        switch (in_const) {
            case constant::undefined: return "N/A";
            case constant::zero: return "0";
            case constant::one: return "1";
            case constant::two: return "2";
            case constant::pi: return "pi";
            case constant::twopi: return "2pi";
            case constant::halfpi: return "pi/2";
            case constant::e: return "e";
            case constant::i: return "i";
            default: assert(0); return "";
        }
    } else if (std::holds_alternative<symbol>(in)) {
        return std::get<symbol>(in);
    } else if (std::holds_alternative<placeholder>(in)) {
        return std::string({'a' + static_cast<char>(std::get<placeholder>(in))});
    } else if (std::holds_alternative<empty>(in)) {
        return "";
    }
    assert(0);
    return "";
}

//------------------------------------------------------------------------------
bool match_r(expression const& lhs, expression const& rhs, std::map<placeholder, expression>& placeholders)
{
    // compare placeholders
    if (std::holds_alternative<placeholder>(lhs) && std::holds_alternative<placeholder>(rhs)) {
        return std::get<placeholder>(lhs) == std::get<placeholder>(rhs);
    } else if (std::holds_alternative<placeholder>(lhs) || std::holds_alternative<placeholder>(rhs)) {
        if (std::holds_alternative<placeholder>(lhs)) {
            auto pl = placeholders.find(std::get<placeholder>(lhs));
            if (pl != placeholders.end()) {
                std::map<placeholder, expression> rhs_placeholders = placeholders;
                if (!match_r(pl->second, rhs, rhs_placeholders)) {
                    return false;
                }
                placeholders = rhs_placeholders;
                return true;
            } else {
                placeholders[std::get<placeholder>(lhs)] = rhs;
                return true;
            }
        } else {
            return match_r(rhs, lhs, placeholders);
        }

    // compare values
    } else if (std::holds_alternative<value>(lhs) && std::holds_alternative<value>(rhs)) {
        return std::get<value>(lhs) == std::get<value>(rhs);

    // compare constants
    } else if (std::holds_alternative<constant>(lhs) && std::holds_alternative<constant>(rhs)) {
        return std::get<constant>(lhs) == std::get<constant>(rhs);

    // compare symbols
    } else if (std::holds_alternative<symbol>(lhs) && std::holds_alternative<symbol>(rhs)) {
        return std::get<symbol>(lhs) == std::get<symbol>(rhs);

    // compare ops
    } else if (std::holds_alternative<op>(lhs) && std::holds_alternative<op>(rhs)) {
        op const& lhs_op = std::get<op>(lhs);
        op const& rhs_op = std::get<op>(rhs);

        if (lhs_op.type != rhs_op.type) {
            return false;
        }

        std::map<placeholder, expression> op_placeholders = placeholders;
        if (!match_r(lhs_op.lhs, rhs_op.lhs, op_placeholders)) {
            return false;
        }
        if (!match_r(lhs_op.rhs, rhs_op.rhs, op_placeholders)) {
            return false;
        }
        placeholders = std::move(op_placeholders);
        return true;

    // compare empty
    } else if (std::holds_alternative<empty>(lhs) && std::holds_alternative<empty>(rhs)) {
        return true;

    // no match
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
//! return the total number of operations in the expression
std::size_t op_count(expression const& expr)
{
    if (std::holds_alternative<op>(expr)) {
        op const& expr_op = std::get<op>(expr);
        return 1 + op_count(expr_op.lhs) + op_count(expr_op.rhs);
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------------
void placeholder_count_r(expression const& expr, std::set<placeholder>& placeholders)
{
    if (std::holds_alternative<op>(expr)) {
        op const& expr_op = std::get<op>(expr);
        placeholder_count_r(expr_op.lhs, placeholders);
        placeholder_count_r(expr_op.rhs, placeholders);
    } else if (std::holds_alternative<placeholder>(expr)) {
        placeholders.insert(std::get<placeholder>(expr));
    }
}

//------------------------------------------------------------------------------
//! return the number of unique placeholders in the expression
std::set<placeholder> enumerate_placeholders(expression const& expr)
{
    std::set<placeholder> placeholders;
    placeholder_count_r(expr, placeholders);
    return placeholders;
}

//------------------------------------------------------------------------------
bool match_placeholders(std::map<placeholder, expression> const& map, std::set<placeholder> const& set)
{
    if (map.size() != set.size()) {
        return false;
    }
    for (auto const& p : set) {
        if (map.find(p) == map.end()) {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
bool match(expression const& lhs, expression const& rhs, std::map<placeholder, expression>& placeholders)
{
    std::map<placeholder, expression> expr_placeholders = placeholders;
    if (match_r(lhs, rhs, expr_placeholders)) {
        placeholders = std::move(expr_placeholders);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
bool match(expression const& lhs, expression const& rhs)
{
    std::map<placeholder, expression> placeholders;
    if (match_r(lhs, rhs, placeholders)) {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
expression apply_transform_r(
    expression const& source,
    expression const& target,
    std::map<placeholder, expression> const& placeholders)
{
    // replace placeholders
    if (std::holds_alternative<placeholder>(target)) {
        return placeholders.at(std::get<placeholder>(target));
    } else if (std::holds_alternative<op>(target)) {
        op const& target_op = std::get<op>(target);
        return op{target_op.type, apply_transform_r(source, target_op.lhs, placeholders),
                                  apply_transform_r(source, target_op.rhs, placeholders)};
    } else {
        return target;
    }
}

//------------------------------------------------------------------------------
expression do_transform(expression const& in)
{
    for (auto const& tr : transforms) {
        std::map<placeholder, expression> placeholders;
        if (match(in, tr.source, placeholders)) {
            return apply_transform_r(in, tr.target, placeholders);
        } else if (match(in, tr.target, placeholders)) {
            return apply_transform_r(in, tr.source, placeholders);
        }
    }
    return in;
}

//------------------------------------------------------------------------------
struct token
{
    char const* begin;
    char const* end;

    bool operator==(char const* str) const {
        assert(begin != end);
        return strncmp(begin, str, end - begin) == 0;
    }
    bool operator!=(char const* str) const {
        assert(begin != end);
        return strncmp(begin, str, end - begin) != 0;
    }

    bool operator==(char ch) const {
        return *begin == ch && end == begin + 1;
    }
    bool operator!=(char ch) const {
        return  *begin != ch || end != begin + 1;
    }
};

std::vector<token> tokenize(char const* str)
{
    std::vector<token> tokens;
    while (true) {
        // skip leading whitespace
        while (*str <= ' ') {
            if (!*str) {
                return tokens;
            }
            ++str;
        }

        token t{str, str};
        switch (*str) {
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            case '(':
            case ')':
            case ',':
                ++t.end;
                ++str;
                tokens.push_back(t);
                continue;
        }

        if (*str >= '0' && *str < '9' || *str == '.') {
            bool has_dot = false;
            while (*str >= '0' && *str < '9' || (!has_dot && *str == '.')) {
                if (*str == '.') {
                    has_dot = true;
                }
                ++t.end;
                ++str;
            }
            tokens.push_back(t);
            continue;
        }

        if (*str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z') {
            while (*str >= 'a' && *str <= 'z' || *str >= 'A' && *str <= 'Z') {
                ++t.end;
                ++str;
            }
            tokens.push_back(t);
            continue;
        }
    }

    // never gets here
}

//------------------------------------------------------------------------------
expression parse(token const*& tokens, token const* end);

expression parse_binary_fn(op_type fn, token const*& tokens, token const* end)
{
    assert(*tokens == '(');
    expression lhs = parse(++tokens, end);
    assert(*tokens == ',');
    expression rhs = parse(++tokens, end);
    assert(*tokens == ')');
    ++tokens;
    return op{fn, lhs, rhs};
}

expression parse_unary_fn(op_type fn, token const*& tokens, token const* end)
{
    assert(*tokens == '(');
    expression lhs = parse(++tokens, end);
    assert(*tokens == ')');
    ++tokens;
    return op{fn, lhs};
}

//------------------------------------------------------------------------------
expression parse(token const*& tokens, token const* end)
{
    expression lhs = empty{};

    if (tokens[0] == '(') {
        lhs = parse(++tokens, end);
        assert(*tokens == ')');
        ++tokens;

    //
    //  functions
    //

    } else if (tokens[0] == "ln") {
        ++tokens;
        assert(*tokens == '(');
        lhs = parse(++tokens, end);
        assert(*tokens == ')');
        ++tokens;
        lhs = op{op_type::logarithm, lhs, constant::e};
    } else if (tokens[0] == "log") {
        lhs = parse_binary_fn(op_type::logarithm, ++tokens, end);
    } else if (tokens[0] == "sin") {
        lhs = parse_unary_fn(op_type::sine, ++tokens, end);
    } else if (tokens[0] == "cos") {
        lhs = parse_unary_fn(op_type::cosine, ++tokens, end);
    } else if (tokens[0] == "tan") {
        lhs = parse_unary_fn(op_type::tangent, ++tokens, end);
    } else if (tokens[0] == "sec") {
        lhs = parse_unary_fn(op_type::secant, ++tokens, end);
    } else if (tokens[0] == "csc") {
        lhs = parse_unary_fn(op_type::cosecant, ++tokens, end);
    } else if (tokens[0] == "cot") {
        lhs = parse_unary_fn(op_type::cotangent, ++tokens, end);
    }

    while (tokens < end && tokens[0] != ')' && tokens[0] != ',') {

        //
        //  constants
        //

        if (tokens[0] == '0') {
            ++tokens;
            lhs = constant::zero;
            continue;
        } else if (tokens[0] == '1') {
            ++tokens;
            lhs = constant::one;
            continue;
        } else if (tokens[0] == '2') {
            ++tokens;
            lhs = constant::two;
            continue;
        } else if (tokens[0] == "pi") {
            ++tokens;
            lhs = constant::pi;
            continue;
        } else if (tokens[0] == 'e') {
            ++tokens;
            lhs = constant::e;
            continue;
        } else if (tokens[0] == 'i') {
            ++tokens;
            lhs = constant::i;
            continue;
        }

        //
        //  values
        //

        if (*tokens[0].begin >= '0' && *tokens[0].begin <= '9' || *tokens[0].begin == '.') {
            lhs = std::atof(tokens[0].begin);
            ++tokens;
            continue;
        }

        //
        //  symbols
        //

        if (*tokens[0].begin >= 'a' && *tokens[0].begin <= 'z' || *tokens[0].begin >= 'A' && *tokens[0].begin <= 'Z') {
            lhs = symbol{tokens[0].begin, tokens[0].end};
            ++tokens;
            continue;
        }

        //
        //  operators
        //

        if (tokens[0] == '+') {
            return op{op_type::sum, lhs, parse(++tokens, end)};
        } else if (tokens[0] == '-') {
            return op{op_type::difference, lhs, parse(++tokens, end)};
        } else if (tokens[0] == '*') {
            return op{op_type::product, lhs, parse(++tokens, end)};
        } else if (tokens[0] == '/') {
            return op{op_type::quotient, lhs, parse(++tokens, end)};
        } else if (tokens[0] == '^') {
            return op{op_type::exponent, lhs, parse(++tokens, end)};
        }
    }

    return lhs;
}

//------------------------------------------------------------------------------
expression parse(char const* str)
{
    std::vector<token> tokens = tokenize(str);
    token const* begin = &tokens[0];
    token const* end = begin + tokens.size();
    return parse(begin, end);
}

//------------------------------------------------------------------------------
struct expression_cmp
{
    bool operator()(expression const& lhs, expression const& rhs) const
    {
        return std::less<std::string>()(to_string(lhs), to_string(rhs));
    }
};

//------------------------------------------------------------------------------
struct expression_hash
{
    std::size_t operator()(expression const& expr) const
    {
        return std::hash<std::string>()(to_string(expr));
    }
};

//------------------------------------------------------------------------------
std::set<expression, expression_cmp> enumerate_transforms(expression const& expr)
{
    static std::map<expression, std::set<expression, expression_cmp>, expression_cmp> cached;

    std::set<expression, expression_cmp> out;

    {
        auto it = cached.find(expr);
        if (it != cached.end()) {
            return it->second;
        }
    }

    for (auto const& tr : transforms) {
        std::set<placeholder> source_placeholders = enumerate_placeholders(tr.source);
        std::set<placeholder> target_placeholders = enumerate_placeholders(tr.target);
        std::set<placeholder> merged_placeholders;
        merged_placeholders.insert(source_placeholders.begin(), source_placeholders.end());
        merged_placeholders.insert(target_placeholders.begin(), target_placeholders.end());

        std::map<placeholder, expression> expr_placeholders;

        if (source_placeholders.size() == merged_placeholders.size()) {
            if (match(expr, tr.source, expr_placeholders) && match_placeholders(expr_placeholders, merged_placeholders)) {
                auto expr_tr = apply_transform_r(expr, tr.target, expr_placeholders);
                assert(match(expr_tr, tr.target, expr_placeholders));
                assert(enumerate_placeholders(expr_tr).size() == 0);
                //printf("    %-40s %-20s  =>  %20s\n", to_string(expr_tr).c_str(), to_string(tr.source).c_str(), to_string(tr.target).c_str());
                //for (auto const& pl : expr_placeholders) {
                //    printf("    %50s %s: %s\n", "", to_string(pl.first).c_str(), to_string(pl.second).c_str());
                //}
                out.insert(expr_tr);
            }
        }

        if (target_placeholders.size() == merged_placeholders.size()) {
            if (match(expr, tr.target, expr_placeholders) && match_placeholders(expr_placeholders, merged_placeholders)) {
                auto expr_tr = apply_transform_r(expr, tr.source, expr_placeholders);
                assert(match(expr_tr, tr.source, expr_placeholders));
                assert(enumerate_placeholders(expr_tr).size() == 0);
                //printf("    %-40s %-20s  =>  %20s\n", to_string(expr_tr).c_str(), to_string(tr.target).c_str(), to_string(tr.source).c_str());
                //for (auto const& pl : expr_placeholders) {
                //    printf("    %50s %s: %s\n", "", to_string(pl.first).c_str(), to_string(pl.second).c_str());
                //}
                out.insert(expr_tr);
            }
        }
    }

    // transform subexpressions
    if (std::holds_alternative<op>(expr)) {
        op const& expr_op = std::get<op>(expr);
        auto lhs = enumerate_transforms(expr_op.lhs);
        for (auto const& lhs_tr : lhs) {
            out.insert(op{expr_op.type, lhs_tr, expr_op.rhs});
        }
        auto rhs = enumerate_transforms(expr_op.rhs);
        for (auto const& rhs_tr : rhs) {
            out.insert(op{expr_op.type, expr_op.lhs, rhs_tr});
        }
    }

    cached[expr] = out;
    return out;
}

struct expression_queue_cmp
{
    bool operator()(expression const& lhs, expression const& rhs) const
    {
        return op_count(lhs) > op_count(rhs);
    }
};

//------------------------------------------------------------------------------
void traceback(expression const& expr, std::map<expression, expression, expression_cmp> const& trace)
{
    auto prev = trace.find(expr);
    if (prev != trace.end()) {
        traceback(prev->second, trace);
    }
    printf("(%zu) %s\n", op_count(expr), to_string(expr).c_str());
}

//------------------------------------------------------------------------------
expression simplify(expression const& expr, std::size_t max_operations = SIZE_MAX, std::size_t max_iterations = SIZE_MAX)
{
    std::set<expression, expression_cmp> closed;
    std::priority_queue<expression, std::vector<expression>, expression_queue_cmp> queue;
    std::map<expression, expression, expression_cmp> trace;

    queue.push(expr);
    closed.insert(expr);

    // smallest expression found in search
    expression best = expr;
    std::size_t best_ops = op_count(best);

    for (std::size_t ii = 0; ii < max_iterations && queue.size(); ++ii) {
        auto const next = queue.top();
        queue.pop();
        //printf("%s\n", to_string(next).c_str());

        std::size_t next_ops = op_count(next);
        if (next_ops < best_ops) {
            best = next;
            best_ops = next_ops;
        }

        // exceeded maximum complexity
        if (next_ops >= max_operations) {
            break;
        // can't get any simpler than zero
        } else if (next_ops == 0) {
            break;
        }

        auto transforms = enumerate_transforms(next);
        for (auto const& next_tr : transforms) {
            if (closed.find(next_tr) == closed.end()) {
                queue.push(next_tr);
                closed.insert(next_tr);
                trace[next_tr] = next;
            }
        }
    }

    traceback(best, trace);
    return best;
}

//------------------------------------------------------------------------------
void foo(char const* str)
{
    simplify(parse(str));
}

//------------------------------------------------------------------------------
void bar()
{
    while (true) {
        std::string line; std::getline(std::cin, line);
        if (line == "") {
            break;
        }

        simplify(parse(line.c_str()), 32, 128);
    }
}

//------------------------------------------------------------------------------
int main()
{
    bar();
    return 0;
}
