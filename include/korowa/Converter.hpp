#pragma once
#ifndef KOROWA_CONVERTER_HPP
#define KOROWA_CONVERTER_HPP

#include <bitset>
#include <iomanip>
#include <iostream>
#include <korowa/SyntaxError.hpp>
#include <map>
#include <my/extention/Helpers.hpp>
#include <my/printer/Format.hpp>

namespace korowa {

namespace {

enum Spec {
    Bin,
    Oct,
    Dec,
    Hex,
};

template <class Manipulator>
size_t read(const std::string& number, Manipulator m) {
    std::stringstream ss;
    size_t num;
    ss << m << number;
    ss >> num;
    return num;
}

std::string toBase(Spec from, Spec to, const std::string& number, SyntaxError& error) {
    std::stringstream ss;

    switch (from) {
        case Bin: {
            for (auto&& el : number) {
                if (el != '0' and el != '1') {
                    error = SyntaxError(my::format("Invalid binary digit [{}]", el),
                                        SyntaxError::Type::Converting);
                    return "";
                }
            }

            std::bitset<128> bits(number);
            switch (to) {
                case Bin: {
                    auto str = bits.to_string();
                    return my::trimStart(str, '0');
                }
                case Oct:
                    ss << std::oct << bits.to_ullong();
                    return ss.str();
                case Dec:
                    ss << bits.to_ullong();
                    return ss.str();
                case Hex:
                    ss << std::hex << bits.to_ullong();
                    return ss.str();
            }
        } break;
        case Oct: {
            for (auto&& el : number) {
                if (not std::isdigit(el) or (el - '0') >= 8) {
                    error = SyntaxError(my::format("Invalid octal digit [{}]", el),
                                        SyntaxError::Type::Converting);
                    return "";
                }
            }

            size_t num = read(number, std::oct);

            switch (to) {
                case Bin: {
                    auto str = std::bitset<128>(num).to_string();
                    return my::trimStart(str, '0');
                }
                case Oct:
                    return number;
                case Dec:
                    ss << std::dec << num;
                    return ss.str();
                case Hex:
                    ss << std::hex << num;
                    return ss.str();
            }
        } break;
        case Dec: {
            for (auto&& el : number) {
                if (not std::isdigit(el)) {
                    error = SyntaxError(my::format("Invalid decimal digit [{}]", el),
                                        SyntaxError::Type::Converting);
                    return "";
                }
            }

            size_t num = read(number, std::dec);

            switch (to) {
                case Bin: {
                    auto str = std::bitset<128>(num).to_string();
                    return my::trimStart(str, '0');
                }
                case Oct:
                    ss << std::oct << num;
                    return ss.str();
                case Dec:
                    return number;
                case Hex:
                    ss << std::hex << num;
                    return ss.str();
            }
        } break;
        case Hex: {
            for (auto&& el : number) {
                if (not std::isxdigit(el)) {
                    error = SyntaxError(my::format("Invalid hexadecimal digit [{}]", el),
                                        SyntaxError::Type::Converting);
                    return "";
                }
            }

            size_t num = read(number, std::hex);

            switch (to) {
                case Bin: {
                    auto str = std::bitset<128>(num).to_string();
                    return my::trimStart(str, '0');
                }
                case Oct:
                    ss << std::oct << num;
                    return ss.str();
                case Dec:
                    ss << std::dec << num;
                    return ss.str();
                case Hex:
                    return number;
            }
        } break;
    }
    error = SyntaxError(my::format("Invalid input [{}]", number),
                        SyntaxError::Type::Converting);
    return "";
}

}  // namespace

std::string convert(const std::string& input, SyntaxError& error) {
    static const std::map<std::string, Spec> ops{
        {"bin", Bin},
        {"oct", Oct},
        {"dec", Dec},
        {"hex", Hex},
    };

    Spec from, to;

    std::vector<std::string> buffer;
    buffer.reserve(4);

    std::string curr;
    for (auto&& el : input) {
        if (buffer.size() < 2) {
            if (el == ' ' or el == ':' and curr.size()) {
                buffer.push_back(curr);
                curr.clear();
                continue;
            }
        }
        curr.push_back(el);
    }
    buffer.push_back(curr);

    if (buffer.size() != 3) {
        error = SyntaxError(my::format("Not enough arguments: [{}]", my::join(buffer)),
                            SyntaxError::Type::Parsing);
        return "";
    }

    auto fromIt = ops.find(my::toLower(buffer[0]));
    auto toIt = ops.find(my::toLower(buffer[1]));

    if (fromIt == ops.end()) {
        error = SyntaxError(my::format("Unknown base: [{}]", buffer[0]),
                            SyntaxError::Type::Parsing);
        return "";
    }

    if (toIt == ops.end()) {
        error = SyntaxError(my::format("Unknown base: [{}]", buffer[1]),
                            SyntaxError::Type::Parsing);
        return "";
    }

    from = fromIt->second;
    to = toIt->second;

    return toBase(from, to, buffer[2], error);
}

};  // namespace korowa

#endif  // KOROWA_CONVERTER_HPP