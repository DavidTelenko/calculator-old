#pragma once
#ifndef KOROWA_LEXER_HPP
#define KOROWA_LEXER_HPP

#include <algorithm>
#include <cctype>
#include <cmath>
#include <korowa/SyntaxError.hpp>
#include <limits>
#include <map>
#include <my/extention/Math.hpp>
#include <my/extention/NumParse.hpp>
#include <my/printer/Format.hpp>
#include <numeric>
#include <queue>
#include <stack>
#include <vector>

#define NaN std::numeric_limits<double>::quiet_NaN();

namespace korowa {

namespace detail {

enum Spec {
    Unknown,

    // Punctuation
    LeftPars,
    RightPars,
    Comma,
    LeftArrPars,
    RightArrPars,

    // Specifiers
    Number,
    Variable,

    // Unary operators
    Fact,
    Placeholder1,
    Placeholder2,

    // Binary operators
    Add,
    Sub,
    Div,
    Mul,
    Mod,
    Pow,
    Equals,
    // Unary f-ns
    Sqrt,
    Cbrt,
    Factorial,
    Abs,

    Ln,
    Lg,
    Exp,

    Ceil,
    Floor,
    Round,
    Trunc,

    Sinc,

    Sin,
    Cos,
    Tan,
    Ctan,
    Sinh,
    Cosh,
    Tanh,
    Ctanh,

    Asin,
    Acos,
    Atan,
    Actan,
    Asinh,
    Acosh,
    Atanh,
    Actanh,

    // Binary f-ns
    Min,
    Max,
    Gcd,
    Lcm,
    Log,

    // Constants
    EConst,
    PiConst,
    TauConst,
    PhiConst,

    // Generators
    RndGen,
    PrimeGen,
    TimeGen,
    PlaceholderGen3,
};

struct Token {
    Spec spec = Spec::Unknown;
    std::string value;

    friend std::ostream& operator<<(std::ostream& os, const Token& obj) {
        my::printf(os, "{}", obj.value);
        return os;
    }
};

using TokenContainer = std::vector<Token>;
using TokenQueue = std::deque<Token>;
using TokenStack = std::deque<Token>;

static constexpr bool isOperator(Spec s) { return s >= Fact and s <= Equals; }
static constexpr bool isBinaryOp(Spec s) { return s >= Add and s <= Equals; }
static constexpr bool isUnaryOp(Spec s) { return s >= Fact and s <= Placeholder2; }

static constexpr bool isConstant(Spec s) { return s >= EConst and s <= PhiConst; }
static constexpr bool isGenerator(Spec s) { return s >= RndGen and s <= PlaceholderGen3; }

static constexpr bool isFunction(Spec s) { return s >= Sqrt and s <= Log; }
static constexpr bool isUnaryFn(Spec s) { return s >= Sqrt and s <= Actanh; }
static constexpr bool isBinaryFn(Spec s) { return s >= Min and s <= Log; }

static constexpr uint8_t getPrecedence(Spec s) {
    switch (s) {
        case LeftPars:
        case RightPars:
            return 0;
        case Add:
        case Sub:
            return 1;
        case Div:
        case Mul:
        case Mod:
            return 2;
        case Pow:
            return 3;
    }
    return 4;
}

static constexpr double performUnaryFn(Spec op, double a) {
    switch (op) {
        case Sqrt:
            return std::sqrt(a);
        case Cbrt:
            return std::cbrt(a);
            //
        case Abs:
            return std::fabs(a);
        case Factorial:
        case Fact:
            return std::tgamma(a + 1);
            //
        case Ln:
            return std::log(a);
        case Lg:
            return std::log10(a);
        case Exp:
            return std::exp(a);
            //
        case Ceil:
            return std::ceil(a);
        case Floor:
            return std::floor(a);
        case Round:
            return std::round(a);
        case Trunc:
            return std::trunc(a);
            //
        case Sinc:
            return my::sinc(a);
            //
        case Sin:
            return std::sin(a);
        case Cos:
            return std::cos(a);
        case Tan:
            return std::tan(a);
        case Ctan:
            return std::tan(my::HALF_PI - a);
            //
        case Sinh:
            return std::sinh(a);
        case Cosh:
            return std::cosh(a);
        case Tanh:
            return std::tanh(a);
        case Ctanh:
            return std::tanh(my::HALF_PI - a);
            //
        case Asin:
            return std::asin(a);
        case Acos:
            return std::acos(a);
        case Atan:
            return std::atan(a);
        case Actan:
            return std::atan(my::HALF_PI - a);
            //
        case Asinh:
            return std::asinh(a);
        case Acosh:
            return std::acosh(a);
        case Atanh:
            return std::atanh(a);
        case Actanh:
            return std::atanh(my::HALF_PI - a);
    }
    return NaN;
}

static constexpr double performBinaryFn(Spec op, double a, double b) {
    switch (op) {
        case Add:
            return (a + b);
        case Sub:
            return (a - b);
        case Mul:
            return (a * b);
        case Div:
            return (a / b);
        case Mod:
            return std::fmod(a, b);
        case Pow:
            return std::pow(a, b);
        case Log:
            return std::log(b) / std::log(a);
        case Min:
            return std::min(a, b);
        case Max:
            return std::max(a, b);
        case Gcd:
            return std::gcd((int)a, (int)b);
        case Lcm:
            return std::lcm((int)a, (int)b);
    }
    return NaN;
}

static constexpr double getConstant(Spec op) {
    switch (op) {
        case EConst:
            return my::E;
        case PiConst:
            return my::PI;
        case TauConst:
            return my::TAU;
        case PhiConst:
            return my::PHI;
    }
    return NaN;
}

static constexpr double getGenerated(Spec op) {
    switch (op) {
        case RndGen:
            return my::random();
        case TimeGen:
            return std::time(nullptr);
    }
    return NaN;
}

TokenContainer tokenize(const std::string& expression) {
    static const std::map<char, Spec> ops{
        {'+', Add},
        {'-', Sub},
        {'/', Div},
        {'*', Mul},
        {'%', Mod},
        {'!', Fact},
        {'^', Pow},
        {'(', LeftPars},
        {')', RightPars},
        {'{', LeftPars},
        {'}', RightPars},
        {'[', LeftArrPars},
        {']', RightArrPars},
        {',', Comma},
        {'=', Equals},
    };

    static const std::map<std::string, Spec> funcs{
        {"fact", Factorial},
        //
        {"sqrt", Sqrt},
        {"cbrt", Cbrt},
        //
        {"ln", Ln},
        {"lg", Lg},
        {"exp", Exp},
        //
        {"ceil", Ceil},
        {"floor", Floor},
        {"round", Round},
        {"trunc", Trunc},
        //
        {"sinc", Sinc},
        //
        {"sin", Sin},
        {"cos", Cos},
        {"tan", Tan},
        {"ctan", Ctan},
        //
        {"asin", Asin},
        {"acos", Acos},
        {"atan", Atan},
        {"actan", Actan},
        //
        {"sinh", Sinh},
        {"cosh", Cosh},
        {"tanh", Tanh},
        {"ctanh", Ctanh},
        //
        {"asinh", Asinh},
        {"acosh", Acosh},
        {"atanh", Atanh},
        {"actanh", Actanh},
        //
        {"min", Min},
        {"max", Max},
        {"gcd", Gcd},
        {"lcm", Lcm},
        {"log", Log},
        {"abs", Abs},
    };

    static const std::map<std::string, Spec> consts{
        {"pi", PiConst},
        {"tau", TauConst},
        {"e", EConst},
        {"phi", PhiConst},
    };

    static const std::map<std::string, Spec> gens{
        {"rnd", RndGen},
        {"time", TimeGen},
    };

    enum State {
        OperatorState,
        UnaryOperatorState,
        NumberState,
        FractionState,
        VariableState,
        FunctionState,
        BeginState,
        ReadState,
    } state = BeginState;

    TokenContainer tokens;
    tokens.reserve(expression.size());

    for (const auto& ch : expression) {
        auto& last = tokens.back();
        const auto conv = std::string(1, ch);

        switch (state) {
            case BeginState: {
                if (std::isdigit(ch)) {
                    tokens.push_back({Number, conv});
                    state = NumberState;
                    break;
                }
                if (ch == '.') {
                    tokens.push_back({Number, conv});
                    state = FractionState;
                    break;
                }
                if (std::isalpha(ch)) {
                    state = VariableState;

                    if (auto it = consts.find(conv); it != consts.end()) {
                        tokens.push_back({it->second, conv});
                        break;
                    }

                    if (auto it = gens.find(conv); it != gens.end()) {
                        tokens.push_back({it->second, conv});
                        break;
                    }

                    tokens.push_back({Variable, conv});
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    state = OperatorState;

                    if (it->second == Sub) {
                        tokens.push_back({Number, "-1"});
                        tokens.push_back({Mul, "*"});
                        break;
                    }

                    if (it->second == Add) break;

                    tokens.push_back({it->second, conv});
                    break;
                }
                if (!std::isblank(ch)) {
                    tokens.push_back({Unknown, conv});
                }
                break;
            }
            case ReadState: {
                if (std::isdigit(ch)) {
                    tokens.push_back({Number, conv});
                    state = NumberState;
                    break;
                }
                if (ch == '.') {
                    tokens.push_back({Number, conv});
                    state = FractionState;
                    break;
                }
                if (std::isalpha(ch)) {
                    state = VariableState;

                    if (auto it = consts.find(conv); it != consts.end()) {
                        tokens.push_back({it->second, conv});
                        break;
                    }

                    if (auto it = gens.find(conv); it != gens.end()) {
                        tokens.push_back({it->second, conv});
                        break;
                    }

                    tokens.push_back({Variable, conv});
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    state = OperatorState;
                    tokens.push_back({it->second, conv});
                    break;
                }
                if (!std::isblank(ch)) {
                    tokens.push_back({Unknown, conv});
                }
                break;
            }
            case NumberState: {
                if (std::isdigit(ch)) {
                    last.value.push_back(ch);
                    break;
                }
                if (ch == '.') {
                    last.value.push_back(ch);
                    state = FractionState;
                    break;
                }
                if (std::isalpha(ch)) {
                    tokens.push_back({Mul, "*"});
                    tokens.push_back({Variable, conv});
                    state = VariableState;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    if (it->second == LeftPars) tokens.push_back({Mul, "*"});
                    tokens.push_back({it->second, conv});
                    state = OperatorState;
                    break;
                }
                if (ch == '\'') {
                    break;
                }
                if (std::isblank(ch)) {
                    state = ReadState;
                    break;
                }
                tokens.push_back({Unknown, conv});
                break;
            }

            case FractionState: {
                if (std::isdigit(ch)) {
                    last.value.push_back(ch);
                    break;
                }
                if (std::isalpha(ch)) {
                    tokens.push_back({Mul, "*"});
                    tokens.push_back({Variable, conv});
                    state = VariableState;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    if (it->second == LeftPars) tokens.push_back({Mul, "*"});
                    tokens.push_back({it->second, conv});
                    state = OperatorState;
                    break;
                }
                if (std::isblank(ch)) {
                    state = ReadState;
                    break;
                }
                tokens.push_back({Unknown, conv});
                break;
            }

            case VariableState: {
                if (std::isdigit(ch)) {
                    last.value.push_back(ch);
                    break;
                }
                if (std::isalpha(ch)) {
                    last.value.push_back(ch);

                    if (auto it = funcs.find(last.value); it != funcs.end()) {
                        last.spec = it->second;
                        state = FunctionState;
                        break;
                    }

                    if (auto it = consts.find(last.value); it != consts.end()) {
                        last.spec = it->second;
                        break;
                    }

                    if (auto it = gens.find(last.value); it != gens.end()) {
                        last.spec = it->second;
                        break;
                    }

                    last.spec = Variable;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    tokens.push_back({it->second, conv});
                    state = OperatorState;

                    if (auto cit = consts.find(last.value); cit != consts.end()) {
                        last.spec = cit->second;
                        break;
                    }

                    if (auto cit = gens.find(last.value); cit != gens.end())
                        last.spec = cit->second;

                    break;
                }
                if (std::isblank(ch)) {
                    state = ReadState;
                    break;
                }
                tokens.push_back({Unknown, conv});
                break;
            }
            case OperatorState: {
                if (std::isdigit(ch)) {
                    tokens.push_back({Number, conv});
                    state = NumberState;
                    break;
                }
                if (ch == '.') {
                    tokens.push_back({Number, conv});
                    state = FractionState;
                    break;
                }
                if (std::isalpha(ch)) {
                    tokens.push_back({Variable, conv});
                    state = VariableState;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    auto& curr = it->second;
                    auto& prev = last.spec;
                    if (curr == LeftPars and prev == RightPars) {
                        tokens.push_back({Mul, "*"});
                        break;
                    }
                    if (curr == Mul and prev == Mul) {
                        prev = Pow;
                        last.value.push_back('*');
                        break;
                    }
                    if (curr == Sub and prev != RightPars and not isUnaryOp(prev)) {
                        tokens.push_back({Number, "-1"});
                        tokens.push_back({Mul, "*"});
                        state = UnaryOperatorState;
                        break;
                    }
                    if (curr == Add and prev != RightPars and not isUnaryOp(prev)) {
                        state = UnaryOperatorState;
                        break;
                    }

                    tokens.push_back({it->second, conv});
                    break;
                }
                if (!std::isblank(ch)) {
                    tokens.push_back({Unknown, conv});
                }
                break;
            }
            case UnaryOperatorState: {
                if (std::isdigit(ch)) {
                    tokens.push_back({Number, conv});
                    state = NumberState;
                    break;
                }
                if (ch == '.') {
                    tokens.push_back({Number, conv});
                    state = FractionState;
                    break;
                }
                if (std::isalpha(ch)) {
                    tokens.push_back({Variable, conv});
                    state = VariableState;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    if (it->second == LeftPars)
                        state = OperatorState;
                    tokens.push_back({it->second, conv});
                    break;
                }
                if (std::isblank(ch)) {
                    state = ReadState;
                    break;
                }
                tokens.push_back({Unknown, conv});
                break;
            }
            case FunctionState: {
                if (std::isdigit(ch) or std::isalpha(ch)) {
                    last.value.push_back(ch);
                    // is it still a function or some unknown stuff
                    if (auto it = funcs.find(last.value); it != funcs.end()) {
                        last.spec = it->second;
                        break;
                    }
                    last.spec = Variable;
                    state = VariableState;
                    break;
                }
                if (auto it = ops.find(ch); it != ops.end()) {
                    tokens.push_back({it->second, conv});
                    state = OperatorState;
                    break;
                }
                if (!std::isblank(ch)) {
                    tokens.push_back({Unknown, conv});
                    break;
                }
                break;
            }
        }
    }

    return tokens;
}

TokenQueue parse(const TokenContainer& tokens, SyntaxError& err) {
    TokenQueue output;
    TokenStack operators;

    if (auto it = std::find_if(tokens.begin(), tokens.end(),
                               [](auto el) { return el.spec == Unknown; });
        it != tokens.end()) {
        err = SyntaxError(
            my::format("Unknown symbol: [{}] (:{})",
                       it->value, std::distance(tokens.begin(), it)),
            SyntaxError::Type::UnknownToken, {it->value});
        return output;
    }

    size_t index = 0;
    bool arrExpr = false;
    for (const auto& token : tokens) {
        if (token.spec == Number or token.spec == Variable or
            isConstant(token.spec) or isGenerator(token.spec))
            output.push_back(token);

        else if (isUnaryOp(token.spec))
            output.push_back(token);

        else if (isFunction(token.spec) or token.spec == LeftPars)
            operators.push_back(token);

        else if (isBinaryOp(token.spec)) {
            while (not operators.empty() and
                   getPrecedence(operators.back().spec) >= getPrecedence(token.spec)) {
                output.push_back(operators.back());
                operators.pop_back();
            }
            operators.push_back(token);
        }

        else if (token.spec == Comma) {
            if (operators.empty()) {
                err = SyntaxError(
                    my::format("Mismatched parenthesis or function argument separators (,) (:{})",
                               index),
                    SyntaxError::Type::Parsing);
                return output;
            }
            while (operators.back().spec != LeftPars) {
                output.push_back(operators.back());
                operators.pop_back();
                if (operators.empty()) {
                    err = SyntaxError(
                        my::format("Mismatched parenthesis or function argument separators (,) (:{})",
                                   index),
                        SyntaxError::Type::Parsing);
                    return output;
                }
            }
        }

        else if (token.spec == RightPars) {
            if (operators.empty()) {
                err = SyntaxError(my::format("Mismatched parenthesis (:{})", index),
                                  SyntaxError::Type::Parsing);
                return output;
            }
            while (operators.back().spec != LeftPars) {
                output.push_back(operators.back());
                operators.pop_back();
                if (operators.empty()) {
                    err = SyntaxError(my::format("Mismatched parenthesis (:{})", index),
                                      SyntaxError::Type::Parsing);
                    return output;
                }
            }
            operators.pop_back();
            if (not operators.empty() and isFunction(operators.back().spec)) {
                output.push_back(operators.back());
                operators.pop_back();
            }
        }

        else if (token.spec == RightArrPars) {
            if (operators.empty()) {
                err = SyntaxError(my::format("Mismatched parenthesis (:{})", index),
                                  SyntaxError::Type::Parsing);
                return output;
            }
            while (operators.back().spec != LeftArrPars) {
                output.push_back(operators.back());
                operators.pop_back();
                if (operators.empty()) {
                    err = SyntaxError(my::format("Mismatched parenthesis (:{})", index),
                                      SyntaxError::Type::Parsing);
                    return output;
                }
            }
            operators.pop_back();
            if (not operators.empty() and isFunction(operators.back().spec)) {
                output.push_back(operators.back());
                operators.pop_back();
            }
        }
        index++;
    }

    while (operators.size()) {
        if (operators.back().spec == LeftPars) {
            err = SyntaxError(my::format("Mismatched parenthesis (:{})", index),
                              SyntaxError::Type::Parsing);
            return output;
        }
        output.push_back(operators.back());
        operators.pop_back();
    }

    return output;
}

}  // namespace detail

/**
 * @brief Evaluates math expression with error handling and variable dumping.
 * It is only possible to assign lhs value to rhs number or other variable
 *
 * @param input string representing math expression
 * @param err occurred error reference
 * @param variables reference to map of saved variables
 * @return double evaluated result
 */
double eval(const std::string& input, SyntaxError& err,
            std::map<std::string, double>& variables) {
    using namespace detail;

    /////////////forced sacrifice////////////////
#define getNumOrError(name)                                                   \
    if (evalStack.empty()) {                                                  \
        err = SyntaxError("Evaluation error", SyntaxError::Type::Evaluation); \
        return NaN;                                                           \
    }                                                                         \
    auto name = evalStack.back();                                             \
    evalStack.pop_back();
    ////////////////////////////////////////////

    auto tokens = tokenize(input);
    auto tokenQueue = parse(tokens, err);
#ifdef KOROWA_PRINT_TOKENS
    my::printf("\n  {}\n", my::join(tokens, "| |", "[|", "|]"));
    my::printf("\n  {}\n", my::join(tokenQueue, "| |", "[|", "|]"));
#endif

    if (err) return NaN;
    if (tokenQueue.empty()) {
        err = SyntaxError("Empty expression",
                          SyntaxError::Type::Evaluation);
        return NaN;
    }

    std::vector<double> evalStack;
    std::string variable{};

    if (tokenQueue.front().spec == Variable and
        tokenQueue.back().spec == Equals) {
        variable = tokenQueue.front().value;
        tokenQueue.pop_front();
    }

    while (not tokenQueue.empty()) {
        const auto currSpec = tokenQueue.front().spec;
        const auto currVal = tokenQueue.front().value;

        if (currSpec == Equals) {
            if (evalStack.size() > 1) {
                err = SyntaxError(my::format("Redundant values: {}",
                                             my::join(evalStack)),
                                  SyntaxError::Type::Evaluation);
                return NaN;
            }
            if (variable.empty()) {
                err = SyntaxError("Inapropriate use of = operator: trying to assign to \"\"",
                                  SyntaxError::Type::Evaluation);
                return NaN;
            }
            variables[variable] = evalStack.back();
            return evalStack.back();
        }

        if (isBinaryFn(currSpec) or isBinaryOp(currSpec)) {
            getNumOrError(b);
            getNumOrError(a);

            evalStack.push_back(performBinaryFn(currSpec, a, b));
            tokenQueue.pop_front();
        }

        else if (isUnaryFn(currSpec) or isUnaryOp(currSpec)) {
            getNumOrError(a);

            evalStack.push_back(performUnaryFn(currSpec, a));
            tokenQueue.pop_front();
        }

        else if (isConstant(currSpec)) {
            evalStack.push_back(getConstant(currSpec));
            tokenQueue.pop_front();
        }

        else if (isGenerator(currSpec)) {
            evalStack.push_back(getGenerated(currSpec));
            tokenQueue.pop_front();
        }

        else if (currSpec == Number) {
            evalStack.push_back(my::parse<double>(currVal));
            tokenQueue.pop_front();
        }

        else if (currSpec == Variable) {
            if (auto it = variables.find(currVal); it != variables.end()) {
                evalStack.push_back(it->second);
                tokenQueue.pop_front();
                continue;
            }
            err = SyntaxError(my::format("Unknown variable: [{}]", currVal),
                              SyntaxError::Type::UnknownToken, {currVal});
            return NaN;
        }
    }

    if (evalStack.size() > 1) {
        err = SyntaxError(my::format("Redundant values: {}", my::join(evalStack)),
                          SyntaxError::Type::Evaluation);
        return NaN;
    }

#undef getNumOrError
    return evalStack.back();  // top
}

/**
 * @brief Evaluates math expression with error handling.
 *
 * @param input string representing math expression
 * @param err occurred error reference
 * @return double evaluated result
 */
double eval(const std::string& input, SyntaxError& err) {
    using namespace detail;

    /////////////forced sacrifice////////////////
#define getNumOrError(name)                                                   \
    if (evalStack.empty()) {                                                  \
        err = SyntaxError("Evaluation error", SyntaxError::Type::Evaluation); \
        return NaN;                                                           \
    }                                                                         \
    auto name = evalStack.back();                                             \
    evalStack.pop_back();
    ////////////////////////////////////////////

    auto tokens = tokenize(input);
    auto tokenQueue = parse(tokens, err);
#ifdef KOROWA_PRINT_TOKENS
    my::printf("\n  {}\n", my::join(tokens, "| |", "[|", "|]"));
    my::printf("\n  {}\n", my::join(tokenQueue, "| |", "[|", "|]"));
#endif

    if (err) return NaN;
    if (tokenQueue.empty()) {
        err = SyntaxError("Empty expression",
                          SyntaxError::Type::Evaluation);
        return NaN;
    }

    std::vector<double> evalStack;

    while (not tokenQueue.empty()) {
        const auto currSpec = tokenQueue.front().spec;
        const auto currVal = tokenQueue.front().value;

        if (isBinaryFn(currSpec) or isBinaryOp(currSpec)) {
            getNumOrError(b);
            getNumOrError(a);

            evalStack.push_back(performBinaryFn(currSpec, a, b));
            tokenQueue.pop_front();
        }

        else if (isUnaryFn(currSpec) or isUnaryOp(currSpec)) {
            getNumOrError(a);

            evalStack.push_back(performUnaryFn(currSpec, a));
            tokenQueue.pop_front();
        }

        else if (isConstant(currSpec)) {
            evalStack.push_back(getConstant(currSpec));
            tokenQueue.pop_front();
        }

        else if (isGenerator(currSpec)) {
            evalStack.push_back(getGenerated(currSpec));
            tokenQueue.pop_front();
        }

        else if (currSpec == Number) {
            evalStack.push_back(my::parse<double>(currVal));
            tokenQueue.pop_front();
        }

        else if (currSpec == Variable) {
            err = SyntaxError(my::format("Unknown variable (variables may be disabled): [{}]", currVal),
                              SyntaxError::Type::UnknownToken, {currVal});
            return NaN;
        }
    }

    if (evalStack.size() > 1) {
        err = SyntaxError(my::format("Redundant values: {}", my::join(evalStack)),
                          SyntaxError::Type::Evaluation);
        return NaN;
    }

#undef getNumOrError
    return evalStack.back();  // top
}

/**
 * @brief Evaluates math expression without error handling and variable dumping.
 * Simplified version for in-code usage.
 *
 * @param input string representing math expression
 * @return double evaluated result
 */
double eval(const std::string& input) {
    using namespace detail;

    /////////////forced sacrifice////////////////
#define getNumOrError(name)       \
    if (evalStack.empty())        \
        return NaN;               \
    auto name = evalStack.back(); \
    evalStack.pop_back();
    ////////////////////////////////////////////

    korowa::SyntaxError err;
    auto tokens = tokenize(input);
    auto tokenQueue = parse(tokens, err);

    if (err) return NaN;
    if (tokenQueue.empty()) {
        return NaN;
    }

    std::vector<double> evalStack;
    std::string variable{};

    while (not tokenQueue.empty()) {
        const auto currSpec = tokenQueue.front().spec;
        const auto currVal = tokenQueue.front().value;

        if (currSpec == Equals) {
            return NaN;
        }

        if (isBinaryFn(currSpec) or isBinaryOp(currSpec)) {
            getNumOrError(b);
            getNumOrError(a);

            evalStack.push_back(performBinaryFn(currSpec, a, b));
            tokenQueue.pop_front();
        }

        else if (isUnaryFn(currSpec) or isUnaryOp(currSpec)) {
            getNumOrError(a);

            evalStack.push_back(performUnaryFn(currSpec, a));
            tokenQueue.pop_front();
        }

        else if (isConstant(currSpec)) {
            evalStack.push_back(getConstant(currSpec));
            tokenQueue.pop_front();
        }

        else if (isGenerator(currSpec)) {
            evalStack.push_back(getGenerated(currSpec));
            tokenQueue.pop_front();
        }

        else if (currSpec == Number) {
            evalStack.push_back(my::parse<double>(currVal));
            tokenQueue.pop_front();
        }

        else if (currSpec == Variable) {
            return NaN;
        }
    }

    if (evalStack.size() > 1) {
        return NaN;
    }

#undef getNumOrError
    return evalStack.back();  // top
}

#undef NaN

}  // namespace korowa
#endif  // LEXER_HPP