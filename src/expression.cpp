// expression.cpp
//

#include "expression.h"
#include "parser.h"

#include <cassert>
#include <map>
#include <queue>
#include <set>

namespace algebra {

char const* transform_strings[] = {
    // associativity of addition
    "(x + y) + z = x + (y + z)",

    // associativity of multiplication
    "(x * y) * z = x * (y * z)",

    // commutativity of addition
    "x + y = y + x",

    // commutativity of multiplication
    "x * y = y * x",

    // distributivity of multiplication over addition
    "a * (x + y) = a * x + a * y",

    // additive identity
    "x + 0 = x",

    // multiplicative identity
    "x * 1 = x",

    // multiplicative kernel
    "x * 0 = 0",

    // additive inverse
    "x + (-x) = 0",
    "-x = 0 - x",
    "x + (-y) = x - y",

    // multiplicative inverse
    "x * (x^-1) = 1",
    "1/x = 1 / x",
    "x * (1/y) = x / y",

    "x + x = x * 2",
    "x * x = x ^ 2",

    //
    //  exponentiation and logarithms
    //

    "log(x * y, b) = log(x, b) + log(y, b)",

    // change of base
    "log(x, b) = log(x, y) / log(b, y)",

    "b ^ log(x, b) = x",

    // exponentiation identity
    "b ^ x * b ^ y = b ^ (x + y)",

    "(b ^ x) ^ y = b ^ (x * y)",

    // distributivity over multiplication
    "(x * y) ^ n = (x ^ n) * (y ^ n)",

    "x ^ 0 = 1",

    "x ^ 1 = x",

    "log(1, x) = 0",

    // function equivalence
    "log(x, e) = ln(x)",
    "log(x, y) = ln(x) / ln(y)",

    "e ^ x = exp(x)",
    "a ^ x = exp(x * ln(a))",

    //
    //  complex numbers
    //

    // fundamental property of i
    "i ^ 2 = -1",
    // euler's formula
    "e ^ (i * x) = cos(x) + i * sin(x)",

    //
    //  trigonometry
    //

    "sin(0) = 0",
    "cos(0) = 1",
    "sin(pi/2) = 1",
    "cos(pi/2) = 0",

    "tan(x) = sin(x) / cos(x)",
    "sec(x) = 1 / cos(x)",
    "csc(x) = 1 / sin(x)",
    "cot(x) = 1 / tan(x)",
    "1 = sin(x) ^ 2 + cos(x) ^ 2",

    "sin(-x) = -sin(x)",
    "cos(-x) = cos(x)",
    "tan(-x) = -tan(x)",

    "sin(pi/2 - x) = cos(x)",
    "cos(pi/2 - x) = sin(x)",
    "tan(pi/2 - x) = cot(x)",

    "sin(pi - x) = sin(x)",
    "cos(pi - x) = -cos(x)",
    "tan(pi - x) = -tan(x)",

    "sin(2pi - x) = sin(-x)",
    "cos(2pi - x) = cos(-x)",
    "tan(2pi - x) = tan(-x)",

    "sin(x + y) = sin(x) * cos(y) + cos(x) * sin(y)",

    "sin(x - y) = sin(x) * cos(y) - cos(x) * sin(y)",

    "cos(x + y) = cos(x) * cos(y) - sin(x) * sin(y)",
    "cos(x - y) = cos(x) * cos(y) + sin(x) * sin(y)",

    "sin(2pi + x) = sin(x)",
    "cos(2pi + x) = cos(x)",
    "tan(2pi + x) = tan(x)",

    "sin(2x) = 2 * sin(x) * cos(x)",
    "cos(2x) = cos(x) ^ 2 - sin(x) ^ 2",
    "cos(2x) = 2 * cos(x) ^ 2 - 1",

    "sin(3x) = 3 * sin(x) - 4 * sin(x) ^ 3",
    "cos(3x) = 4 * cos(x) ^ 3 - 3 * cos(x)",

    "sin(x) ^ 2 = (1 - cos(2x)) / 2",
    "cos(x) ^ 2 = (1 + cos(2x)) / 2",

    //
    //  differentiation
    //

    "d/dx(f + g) = d/dx(f) + d/dx(g)",
    "d/dx(f - g) = d/dx(f) - d/dx(g)",

    // product rule
    "d/dx(f * g) = d/dx(f) * g + f * d/dx(g)",

    // quotient rule
    "d/dx(f / g) = (d/dx(f) * g - f * d/dx(g)) / g^2",

    // chain rule
    //"d/dx(f(g)) = d/dx(f)(g) * d/dx(g)",

    // power rule
    "d/dx(x) = 1",
    "d/dx(x ^ r) = r * x ^ (r - 1)", // (r != 0),

    "d/dx(ln(x)) = 1/x",
    "d/dx(ln(f)) = d/dx(f) / x",
    "d/dx(exp(x)) = exp(x)",
    "d/dx(exp(f)) = d/dx(f) * exp(f)",

    "d/dx(sin(x)) = cos(x)",
    "d/dx(cos(x)) = -sin(x)",
    "d/dx(tan(x)) = sec(x) ^ 2",

    "d/dx(sin(f)) = d/dx(f) * cos(f)",
    "d/dx(cos(f)) = d/dx(f) * -sin(f)",
    "d/dx(tan(f)) = d/dx(f) * sec(f) ^ 2",
};

std::vector<transform> transforms;

//------------------------------------------------------------------------------
std::string to_string(expression const& in)
{
    if (std::holds_alternative<op>(in)) {
        op const& in_op = std::get<op>(in);
        switch (in_op.type) {
            case op_type::function: {
                if (std::holds_alternative<function>((expression const&)in_op.lhs)) {
                    function fn = std::get<function>((expression const&)in_op.lhs);
                    switch (fn) {
                        case function::exponent: return std::string("exp(") + to_string(in_op.rhs) + ")";
                        case function::logarithm: return std::string("ln(") + to_string(in_op.rhs) + ")";
                        case function::sine: return std::string("sin(") + to_string(in_op.rhs) + ")";
                        case function::cosine: return std::string("cos(") + to_string(in_op.rhs) + ")";
                        case function::tangent: return std::string("tan(") + to_string(in_op.rhs) + ")";
                        case function::secant: return std::string("sec(") + to_string(in_op.rhs) + ")";
                        case function::cosecant: return std::string("csc(") + to_string(in_op.rhs) + ")";
                        case function::cotangent: return std::string("cot(") + to_string(in_op.rhs) + ")";
                        default: assert(0); return "";
                    }
                } else if (std::holds_alternative<symbol>((expression const&)in_op.lhs)
                        || std::holds_alternative<placeholder>((expression const&)in_op.lhs)) {
                    return to_string(in_op.lhs) + "(" + to_string(in_op.rhs) + ")";
                } else {
                    assert(0); return "";
                }
            }
            case op_type::comma: return to_string(in_op.lhs) + ", " + to_string(in_op.rhs);
            case op_type::equality: return to_string(in_op.lhs) + " = " + to_string(in_op.rhs);
            case op_type::sum: return std::string("(") + to_string(in_op.lhs) + " + " + to_string(in_op.rhs) + ")";
            case op_type::difference: return std::string("(") + to_string(in_op.lhs) + " - " + to_string(in_op.rhs) + ")";
            case op_type::negative: return std::string("(-") + to_string(in_op.lhs) + ")";
            case op_type::product: return std::string("(") + to_string(in_op.lhs) + " * " + to_string(in_op.rhs) + ")";
            case op_type::quotient: return std::string("(") + to_string(in_op.lhs) + " / " + to_string(in_op.rhs) + ")";
            case op_type::reciprocal: return std::string("(1/") + to_string(in_op.lhs) + ")";
            case op_type::exponent: return std::string("(") + to_string(in_op.lhs) + " ^ " + to_string(in_op.rhs) + ")";
            case op_type::logarithm: return std::string("log(") + to_string(in_op.lhs) + ", " + to_string(in_op.rhs) + ")";
            case op_type::derivative: return std::string("d/d") + to_string(in_op.lhs) + "(" + to_string(in_op.rhs) + ")";
            default: assert(0); return "";
        }
    } else if (std::holds_alternative<value>(in)) {
        char buf[256];
        std::snprintf(buf, 256, "%g", std::get<value>(in));
        return buf;
    } else if (std::holds_alternative<constant>(in)) {
        constant const& in_const = std::get<constant>(in);
        switch (in_const) {
            case constant::undefined: return "N/A";
            case constant::pi: return "pi";
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

    // compare functions
    } else if (std::holds_alternative<function>(lhs) && std::holds_alternative<function>(rhs)) {
        return std::get<function>(lhs) == std::get<function>(rhs);

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
int compare(expression const& lhs, expression const& rhs)
{
    // compare expression types
    std::ptrdiff_t d = lhs.index() - rhs.index();
    if (d < 0) {
        return -1;
    } else if (d == 0) {
        // empty ops are all equal
        if (std::holds_alternative<empty>(lhs)) {
            assert(std::holds_alternative<empty>(rhs));
            return 0;
        // compare operations recursively
        } else if (std::holds_alternative<op>(lhs)) {
            assert(std::holds_alternative<op>(rhs));
            op const& lhs_op = std::get<op>(lhs);
            op const& rhs_op = std::get<op>(rhs);
            // compare operator by enum value
            if (lhs_op.type < rhs_op.type) {
                return -1;
            } else if (lhs_op.type == rhs_op.type) {
                // compare left operands
                int d2 = compare(lhs_op.lhs, rhs_op.lhs);
                if (d2 < 0) {
                    return -1;
                } else if (d2 == 0) {
                    // compare right operands
                    return compare(lhs_op.rhs, rhs_op.rhs);
                } else {
                    return 1;
                }
            } else {
                return 1;
            }
        // compare functions by enum value
        } else if (std::holds_alternative<function>(lhs)) {
            assert(std::holds_alternative<function>(rhs));
            return static_cast<int>(std::get<function>(lhs)) - static_cast<int>(std::get<function>(rhs));
        // compare constants by enum value
        } else if (std::holds_alternative<constant>(lhs)) {
            assert(std::holds_alternative<constant>(rhs));
            return static_cast<int>(std::get<constant>(lhs)) - static_cast<int>(std::get<constant>(rhs));
        // compare values by value
        } else if (std::holds_alternative<value>(lhs)) {
            assert(std::holds_alternative<value>(rhs));
            double dv = std::get<value>(lhs) - std::get<value>(rhs);
            if (dv < 0.0) {
                return -1;
            } else if (dv == 0.0) {
                return 0;
            } else {
                return 1;
            }
        // compare symbols lexicographically
        } else if (std::holds_alternative<symbol>(lhs)) {
            assert(std::holds_alternative<symbol>(rhs));
            return std::get<symbol>(lhs).compare(std::get<symbol>(rhs));
        // compare placeholders by enum value
        } else if (std::holds_alternative<placeholder>(lhs)) {
            assert(std::holds_alternative<placeholder>(rhs));
            return static_cast<int>(std::get<placeholder>(lhs)) - static_cast<int>(std::get<placeholder>(rhs));
        } else {
            assert(0);
            return 0;
        }
    } else {
        return 1;
    }
}

//------------------------------------------------------------------------------
struct expression_cmp
{
    bool operator()(expression const& lhs, expression const& rhs) const
    {
        return compare(lhs, rhs) < 0;
    }
};

//------------------------------------------------------------------------------
// convert symbols into placeholders so they can be used for substitution
expression convert_placeholders(expression const& expr)
{
    if (std::holds_alternative<op>(expr)) {
        op const& expr_op = std::get<op>(expr);
        return op{expr_op.type, convert_placeholders(expr_op.lhs), convert_placeholders(expr_op.rhs)};
    } else if (std::holds_alternative<symbol>(expr)) {
        symbol s = std::get<symbol>(expr);
        assert(s.length() == 1 && s[0] >= 'a' && s[0] <= 'z');
        return static_cast<placeholder>(static_cast<int>(placeholder::x) + (s[0] - 'a'));
    } else {
        return expr;
    }
}

//------------------------------------------------------------------------------
bool resolve_transforms()
{
    if (transforms.size()) {
        return true;
    }

    for (char const* str : transform_strings) {
        expression expr = parse(str);

        assert(std::holds_alternative<op>(expr));
        assert(std::get<op>(expr).type == op_type::equality);

        op const& expr_op = std::get<op>(expr);
        transforms.push_back({convert_placeholders(expr_op.lhs), convert_placeholders(expr_op.rhs)});
    }

    return true;
}

//------------------------------------------------------------------------------
std::set<expression, expression_cmp> enumerate_transforms(expression const& expr)
{
    static std::map<expression, std::set<expression, expression_cmp>, expression_cmp> cached;

    std::set<expression, expression_cmp> out;

    resolve_transforms();

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

        assert(source_placeholders.size() == merged_placeholders.size()
            || target_placeholders.size() == merged_placeholders.size());

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
        auto lhs_tr = enumerate_transforms(expr_op.lhs);
        for (auto const& tr : lhs_tr) {
            out.insert(op{expr_op.type, tr, expr_op.rhs});
        }
        auto rhs_tr = enumerate_transforms(expr_op.rhs);
        for (auto const& tr : rhs_tr) {
            out.insert(op{expr_op.type, expr_op.lhs, tr});
        }

        // simplify algebraic value expressions
        if (std::holds_alternative<value>((expression const&)expr_op.lhs) && std::holds_alternative<value>((expression const&)expr_op.rhs)) {
            value lhs = std::get<value>((expression const&)expr_op.lhs);
            value rhs = std::get<value>((expression const&)expr_op.rhs);

            switch (expr_op.type) {
                case op_type::sum: out.insert(lhs + rhs); break;
                case op_type::difference:
                    if (lhs < rhs) {
                        out.insert(op{op_type::reciprocal, rhs - lhs});
                    } else {
                        out.insert(lhs - rhs);
                    }
                    break;
                case op_type::product: out.insert(lhs * rhs); break;
                case op_type::quotient: out.insert(lhs / rhs); break;
                case op_type::exponent: out.insert(std::pow(lhs, rhs)); break;
            }
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
expression simplify(expression const& expr, std::size_t max_operations, std::size_t max_iterations)
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

} // namespace algebra
