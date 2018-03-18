// parser.cpp
//

#include "parser.h"

#include <cassert>
#include <algorithm>

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

    template<std::size_t size> bool operator==(char const (&str)[size]) const {
        assert(begin < end);
        return strncmp(begin, str, std::max<std::ptrdiff_t>(end - begin, size - 1)) == 0;
    }

    template<std::size_t size> bool operator!=(char const (&str)[size]) const {
        assert(begin < end);
        return strncmp(begin, str, std::max<std::ptrdiff_t>(end - begin, size - 1)) != 0;
    }

    bool operator==(char ch) const {
        assert(begin < end);
        return *begin == ch && end == begin + 1;
    }

    bool operator!=(char ch) const {
        assert(begin < end);
        return *begin != ch || end != begin + 1;
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
            case '=':
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
result<expression> parse_expression(token const*& tokens, token const* end, int precedence = INT_MAX);
result<expression> parse_operand(token const*& tokens, token const* end);

//------------------------------------------------------------------------------
constexpr int op_precedence(op_type t)
{
    switch (t) {
        case op_type::equality: return 16;
        case op_type::comma: return 17;
        case op_type::sum: return 6;
        case op_type::difference: return 6;
        case op_type::product: return 5;
        case op_type::quotient: return 5;
        case op_type::exponent: return 4;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
result<op_type> parse_operator(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected operator after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')') {
        return error{*tokens, "expected operator after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (tokens[0] == '=') {
        ++tokens;
        return op_type::equality;
    } else if (tokens[0] == ',') {
        ++tokens;
        return op_type::comma;
    } else if (tokens[0] == '+') {
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

    result<expression> lhs = parse_expression(tokens, end);
    if (std::holds_alternative<error>(lhs)) {
        return std::get<error>(lhs);
    }

    if (!std::holds_alternative<op>((expression const&)lhs)) {
        return error{};
    }

    op const& lhs_op = std::get<op>((expression const&)lhs);
    if (lhs_op.type != op_type::comma) {
        return error{};
    }

    if (tokens >= end) {
        return error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
    } else if (*tokens != ')') {
        return error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    } else {
        ++tokens;
    }

    return op{type, lhs_op.lhs, lhs_op.rhs};
}

//------------------------------------------------------------------------------
std::size_t count_arguments(expression const& expr)
{
    if (std::holds_alternative<op>(expr)) {
        op const& expr_op = std::get<op>(expr);
        if (expr_op.type == op_type::comma) {
            return 1 + count_arguments(expr_op.rhs);
        }
    }
    return 1;
}

//------------------------------------------------------------------------------
result<expression> parse_function(token const*& tokens, token const* end, function fn, std::size_t num_args = 1)
{
    token const* start = tokens;
    result<expression> arg = parse_operand(tokens, end);
    if (std::holds_alternative<error>(arg)) {
        return std::get<error>(arg);
    } else if (count_arguments(std::get<expression>(arg)) != num_args) {
        std::size_t n = count_arguments(std::get<expression>(arg));
        return error{{start->begin, (tokens - 1)->end}, "function '" + *(start - 1) + "' does not take " + std::to_string(n) + " arguments"};
    }
    return op{op_type::function, fn, std::get<expression>(arg)};
}

//------------------------------------------------------------------------------
result<expression> parse_operand_explicit(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')') {
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

    } else if (*tokens == "exp") {
        return parse_function(++tokens, end, function::exponent);
    } else if (*tokens == "ln") {
        return parse_function(++tokens, end, function::logarithm);
    } else if (*tokens == "log") {
        return parse_binary_function(++tokens, end, op_type::logarithm);
    } else if (*tokens == "sin") {
        return parse_function(++tokens, end, function::sine);
    } else if (*tokens == "cos") {
        return parse_function(++tokens, end, function::cosine);
    } else if (*tokens == "tan") {
        return parse_function(++tokens, end, function::tangent);
    } else if (*tokens == "sec") {
        return parse_function(++tokens, end, function::secant);
    } else if (*tokens == "csc") {
        return parse_function(++tokens, end, function::cosecant);
    } else if (*tokens == "cot") {
        return parse_function(++tokens, end, function::cotangent);

    //
    //  constants
    //

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
    // derivative
    //

    } else if (end - tokens > 2 && tokens[0] == 'd' && tokens[1] == '/' && tokens[2][0] == 'd') {
        symbol s{tokens[2].begin + 1, tokens[2].end};
        tokens += 3;

        result<expression> expr = parse_operand(tokens, end);
        if (std::holds_alternative<error>(expr)) {
            return std::get<error>(expr);
        }

        return op{op_type::derivative, s, std::get<expression>(expr)};

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
    } else if (*tokens == ')') {
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

        // do not parse `3-x` as `(3)(-x)`
        if (tokens < end && *tokens != '-') {
            result<expression> next = parse_expression(tokens, end, op_precedence(op_type::product));
            if (std::holds_alternative<error>(next)
                || std::holds_alternative<value>(std::get<expression>(next))) {
                tokens = saved;
            } else {
                out = op{op_type::product, out, std::get<expression>(next)};
            }
        }

    // check for function call
    } else if ((std::holds_alternative<op>(out) && std::get<op>(out).type == op_type::derivative)
            || std::holds_alternative<symbol>(out)) {
        if (tokens < end && *tokens == "(") {
            result<expression> args = parse_operand(tokens, end);
            if (std::holds_alternative<error>(args)) {
                return std::get<error>(args);
            }
            out = op{op_type::function, out, std::get<expression>(args)};
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
result<expression> parse_expression(token const*& tokens, token const* end, int precedence)
{
    result<expression> lhs = parse_operand(tokens, end);
    if (std::holds_alternative<error>(lhs)) {
        return std::get<error>(lhs);
    }

    while (tokens < end && *tokens != ')') {
        token const* saved = tokens;

        result<op_type> lhs_op = parse_operator(tokens, end);
        if (std::holds_alternative<error>(lhs_op)) {
            return std::get<error>(lhs_op);
        }

        int lhs_precedence = op_precedence(std::get<op_type>(lhs_op));
        if (precedence <= lhs_precedence && precedence != op_precedence(op_type::comma)) {
            tokens = saved;
            break;
        }

        result<expression> rhs = parse_expression(tokens, end, lhs_precedence);
        if (std::holds_alternative<error>(rhs)) {
            return std::get<error>(rhs);
        } else {
            lhs = op{std::get<op_type>(lhs_op), std::get<expression>(lhs), std::get<expression>(rhs)};
        }
    }

    return lhs;

    {
        // file generates C2783 without this line, compiler bug?
        result<expression> C2783; C2783 = lhs;
    }
}

} // namespace parser

//------------------------------------------------------------------------------
void print_error(char const* str, parser::error const& err)
{
    std::ptrdiff_t offset = err.tok.begin - str;
    std::ptrdiff_t length = err.tok.end - err.tok.begin;
    printf("%s\n", str);
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
