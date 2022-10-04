#pragma once
#ifndef KOROWA_SYNTAX_ERROR_HPP
#define KOROWA_SYNTAX_ERROR_HPP

#include <iostream>
#include <string>
#include <vector>

namespace korowa {

class SyntaxError {
   public:
    enum class Type : uint8_t {
        Undefined,
        Evaluation,
        Converting,
        UnknownToken,
        Parsing,
    };

    explicit SyntaxError() : status(true), mType(Type::Undefined) {}
    explicit SyntaxError(const std::string& message)
        : message(message), status(false), mType(Type::Undefined) {}
    explicit SyntaxError(const std::string& message, Type type)
        : message(message), status(false), mType(type) {}
    explicit SyntaxError(const std::string& message, Type type,
                         const std::vector<std::string>& params)
        : message(message), status(false), mType(type), mParams(params) {}

    friend std::ostream& operator<<(std::ostream& os, const SyntaxError& err) {
        os << (err.message.empty() ? "Ok" : err.message);
        return os;
    }

    operator bool() const { return !status; }
    auto log(std::ostream& os = std::cout) const { os << *this; }
    auto what() const { return message; }
    auto reset() { status = true; }
    const auto type() const { return mType; }
    auto params() const { return mParams; }

   private:
    std::string message{};
    bool status = true;
    SyntaxError::Type mType;
    std::vector<std::string> mParams{};
};

}  // namespace korowa

#endif  // KOROWA_SYNTAX_ERROR_HPP