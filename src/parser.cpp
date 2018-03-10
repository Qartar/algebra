// parser.cpp
//

#include "parser.h"

#include <cassert>

namespace algebra {

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

//------------------------------------------------------------------------------
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

        if (*str >= '0' && *str <= '9' || *str == '.') {
            bool has_dot = false;
            while (*str >= '0' && *str <= '9' || (!has_dot && *str == '.')) {
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

} // namespace algebra
