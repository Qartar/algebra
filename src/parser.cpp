// parser.cpp
//

#include "parser.h"

#include <cassert>

namespace algebra {
namespace parser {

//------------------------------------------------------------------------------
struct token
{
    char const* begin;
    char const* end;

    char operator[](std::size_t index) const {
        assert(end - begin > std::ptrdiff_t(index));
        return begin[index];
    }

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

std::string operator+(char const* lhs, token const& rhs)
{
    return std::string(lhs) + std::string(rhs.begin, rhs.end);
}

std::string operator+(std::string const& lhs, token const& rhs)
{
    return lhs + std::string(rhs.begin, rhs.end);
}

//------------------------------------------------------------------------------
struct error
{
    token tok;
    std::string msg;
};

template<typename T> using result = std::variant<error, T>;

//------------------------------------------------------------------------------
using tokenized = std::vector<token>;

//------------------------------------------------------------------------------
result<tokenized> tokenize(char const* str)
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

        if (*str >= '0' && *str <= '9' || *str == '.') {
            bool has_dot = false;
            while (*str >= '0' && *str <= '9' || (!has_dot && *str == '.')) {
                if (*str == '.') {
                    if (has_dot) {
                        return error{{str, str+1}, "invalid literal"};
                    }
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

        return error{{str, str+1}, "invalid character '" + token{str, str+1} + "'"};
    }

    // never gets here
}

//------------------------------------------------------------------------------
result<expression> parse_expression(token const*& tokens, token const* end);
result<expression> parse_operand(token const*& tokens, token const* end);

//------------------------------------------------------------------------------
result<op_type> parse_operator(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected operator after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected operator after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (tokens[0] == '+') {
        ++tokens;
        return op_type::sum;
    } else if (tokens[0] == '-') {
        ++tokens;
        return op_type::difference;
    } else if (tokens[0] == '*') {
        ++tokens;
        return op_type::product;
    } else if (tokens[0] == '/') {
        ++tokens;
        return op_type::quotient;
    } else if (tokens[0] == '^') {
        ++tokens;
        return op_type::exponent;
    } else {
        return error{*tokens, "expected operator, found '" + *tokens + "'"};
    }
}

//------------------------------------------------------------------------------
result<expression> parse_binary_function(token const*& tokens, token const* end, op_type type)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected '(' after '" + *(tokens - 1) + "'"};
    } else if (*tokens != '(') {
        return error{*tokens, "expected '(' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    } else {
        ++tokens;
    }

    result<expression> lhs = parse_operand(tokens, end);
    if (std::holds_alternative<error>(lhs)) {
        return std::get<error>(lhs);
    }

    if (tokens >= end) {
        return error{*(tokens - 1), "expected ',' after '" + *(tokens - 1) + "'"};
    } else if (*tokens != ',') {
        return error{*tokens, "expected ',' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    } else {
        ++tokens;
    }

    result<expression> rhs = parse_operand(tokens, end);
    if (std::holds_alternative<error>(rhs)) {
        return std::get<error>(rhs);
    }

    if (tokens >= end) {
        return error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
    } else if (*tokens != ')') {
        return error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    } else {
        ++tokens;
    }

    return op{type, std::get<expression>(lhs), std::get<expression>(rhs)};
}

//------------------------------------------------------------------------------
result<expression> parse_unary_function(token const*& tokens, token const* end, op_type type, expression const& rhs = empty{})
{
    result<expression> arg = parse_operand(tokens, end);
    if (std::holds_alternative<error>(arg)) {
        return std::get<error>(arg);
    }
    return op{type, std::get<expression>(arg), rhs};
}

//------------------------------------------------------------------------------
result<expression> parse_operand_explicit(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (*tokens == '(') {
        result<expression> expr = parse_expression(++tokens, end);
        if (std::holds_alternative<error>(expr)) {
            return std::get<error>(expr);
        }
        if (tokens >= end) {
            return error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
        } else if (*tokens != ')') {
            return error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
        }
        ++tokens;
        return std::get<expression>(expr);

    //
    //  functions
    //

    } else if (*tokens == "ln") {
        return parse_unary_function(++tokens, end, op_type::logarithm, constant::e);
    } else if (*tokens == "log") {
        return parse_binary_function(++tokens, end, op_type::logarithm);
    } else if (*tokens == "sin") {
        return parse_unary_function(++tokens, end, op_type::sine);
    } else if (*tokens == "cos") {
        return parse_unary_function(++tokens, end, op_type::cosine);
    } else if (*tokens == "tan") {
        return parse_unary_function(++tokens, end, op_type::tangent);
    } else if (*tokens == "sec") {
        return parse_unary_function(++tokens, end, op_type::secant);
    } else if (*tokens == "csc") {
        return parse_unary_function(++tokens, end, op_type::cosecant);
    } else if (*tokens == "cot") {
        return parse_unary_function(++tokens, end, op_type::cotangent);

    //
    //  constants
    //

    } else if (*tokens == '0') {
        ++tokens;
        return constant::zero;
    } else if (tokens[0] == '1') {
        ++tokens;
        return constant::one;
    } else if (tokens[0] == '2') {
        ++tokens;
        return constant::two;
    } else if (tokens[0] == "pi") {
        ++tokens;
        return constant::pi;
    } else if (tokens[0] == 'e') {
        ++tokens;
        return constant::e;
    } else if (tokens[0] == 'i') {
        ++tokens;
        return constant::i;

    //
    //  values
    //

    } else if (*tokens[0].begin >= '0' && *tokens[0].begin <= '9' || *tokens[0].begin == '.') {
        value v = std::atof(tokens[0].begin);
        ++tokens;
        return v;

    //
    //  symbols
    //

    } else if (*tokens[0].begin >= 'a' && *tokens[0].begin <= 'z' || *tokens[0].begin >= 'A' && *tokens[0].begin <= 'Z') {
        symbol s{tokens[0].begin, tokens[0].end};
        ++tokens;
        return s;

    } else {
        return error{*tokens, "syntax error: '" + *tokens + "'"};
    }
}

//------------------------------------------------------------------------------
result<expression> parse_operand(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    expression out;
    bool is_negative = false;

    // check for negation
    if (*tokens == '-') {
        is_negative = true;
        ++tokens;
    }

    result<expression> operand = parse_operand_explicit(tokens, end);
    if (std::holds_alternative<error>(operand)) {
        return std::get<error>(operand);
    } else {
        out = std::get<expression>(operand);
    }

    // check for implicit multiplication, e.g. `3x`
    if (std::holds_alternative<value>(out) || std::holds_alternative<constant>(out)) {
        token const* saved = tokens;

        result<expression> next = parse_operand_explicit(tokens, end);
        if (std::holds_alternative<error>(next)
            || std::holds_alternative<value>(std::get<expression>(next))) {
            tokens = saved;
        } else {
            out = op{op_type::product, out, std::get<expression>(next)};
        }
    }

    // apply negation after implicit multiplication
    if (is_negative) {
        return op{op_type::negative, out};
    } else {
        return out;
    }
}

//------------------------------------------------------------------------------
constexpr int precedence(op_type t)
{
    switch (t) {
        case op_type::sum: return 6;
        case op_type::difference: return 6;
        case op_type::product: return 5;
        case op_type::quotient: return 5;
        case op_type::exponent: return 4;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
result<expression> parse_expression(token const*& tokens, token const* end)
{
    result<expression> lhs = parse_operand(tokens, end);
    if (std::holds_alternative<error>(lhs)) {
        return std::get<error>(lhs);
    }

    // single operand, return immediately
    if (tokens == end || *tokens == ')' || *tokens == ',') {
        return std::get<expression>(lhs);
    }

    result<op_type> lhs_op = parse_operator(tokens, end);
    if (std::holds_alternative<error>(lhs_op)) {
        return std::get<error>(lhs_op);
    }

    result<expression> mid = parse_operand(tokens, end);
    if (std::holds_alternative<error>(mid)) {
        return std::get<error>(mid);
    }

    // combine operands account for operator precedence
    while (tokens < end) {
        if (*tokens == ')' || *tokens == ',') {
            break;
        }

        result<op_type> rhs_op = parse_operator(tokens, end);
        if (std::holds_alternative<error>(rhs_op)) {
            return std::get<error>(rhs_op);
        }

        result<expression> rhs = parse_operand(tokens, end);
        if (std::holds_alternative<error>(rhs)) {
            return std::get<error>(rhs);
        }

        //  (lhs op mid) op rhs         lhs op (mid op rhs)
        //       v           v    or     v          v
        //      lhs         mid         lhs        mid

        if (precedence(std::get<op_type>(lhs_op)) <= precedence(std::get<op_type>(rhs_op))) {
            lhs = op{std::get<op_type>(lhs_op), std::get<expression>(lhs), std::get<expression>(mid)};
            lhs_op = rhs_op;
            mid = rhs;
        } else {
            mid = op{std::get<op_type>(rhs_op), std::get<expression>(mid), std::get<expression>(rhs)};
        }
    }

    return op{std::get<op_type>(lhs_op), std::get<expression>(lhs), std::get<expression>(mid)};
}

} // namespace parser

//------------------------------------------------------------------------------
void print_error(char const* str, parser::error const& err)
{
    std::ptrdiff_t offset = err.tok.begin - str;
    std::ptrdiff_t length = err.tok.end - err.tok.begin;
    for (std::ptrdiff_t ii = 0; ii < offset; ++ii) {
        printf(" ");
    }
    for (std::ptrdiff_t ii = 0; ii < length; ++ii) {
        printf("^");
    }
    printf(" %s\n", err.msg.c_str());
}

//------------------------------------------------------------------------------
expression parse(char const* str)
{
    parser::result<parser::tokenized> result = parser::tokenize(str);
    if (std::holds_alternative<parser::tokenized>(result)) {
        parser::tokenized const& tokens = std::get<parser::tokenized>(result);
        parser::token const* begin = &tokens[0];
        parser::result<expression> expr = parser::parse_expression(begin, begin + tokens.size());
        if (std::holds_alternative<parser::error>(expr)) {
            print_error(str, std::get<parser::error>(expr));
        } else {
            return std::get<expression>(expr);
        }
    } else if (std::holds_alternative<parser::error>(result)) {
        print_error(str, std::get<parser::error>(result));
    }
    return expression{};
}

} // namespace algebra
