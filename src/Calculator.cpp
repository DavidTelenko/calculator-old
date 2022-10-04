// #define KOROWA_PRINT_TOKENS
#include <direct.h>

#include <fstream>
#include <korowa/Converter.hpp>
#include <korowa/Lexer.hpp>
#include <locale>
#include <my/extention/ConsoleUtils.hpp>
#include <my/extention/File.hpp>
#include <my/printer/ColorFormat.hpp>
#include <my/printer/LightTable.hpp>
#include <my/printer/PrintableBase.hpp>
#include <my/text/Helpers.hpp>
#include <nlohmann/json.hpp>

#define SESSION_FILE "./korowa_session.json"
#define CONFIG_FILE "./korowa_config.json"

class Options {
   public:
    bool alwaysShowHelp = true;
    bool showWelcomeScreen = true;
    bool enableVariables = true;
    bool enableConverters = true;
    bool enableDidYouMean = true;
    bool separateThousands = true;
    bool logEnabled = false;
    // bool enableColors = true;
    size_t precision = 10;
    std::string logTimeFormat = "%H:%M:%S|";  // see https://www.cplusplus.com/reference/ctime/strftime/
    std::string logFilePath = "logs/";
    std::string inputSign = "> ";

    Options() {
        my::File file(CONFIG_FILE);

        if (file.exists()) {
            std::string buffer;
            file.read(buffer);
            const auto read = nlohmann::json::parse(buffer);

            alwaysShowHelp = read["alwaysShowHelp"];
            showWelcomeScreen = read["showWelcomeScreen"];
            enableVariables = read["enableVariables"];
            enableConverters = read["enableConverters"];
            enableDidYouMean = read["enableDidYouMean"];
            separateThousands = read["separateThousands"];
            precision = read["precision"];
            inputSign = read["inputSign"];
            logEnabled = read["logEnabled"];
            logFilePath = read["logFilePath"];
            logTimeFormat = read["logTimeFormat"];
        } else {
            file.create();

            nlohmann::json write;

            write["version"] = "0.2.0";
            write["alwaysShowHelp"] = alwaysShowHelp;
            write["showWelcomeScreen"] = showWelcomeScreen;
            write["enableVariables"] = enableVariables;
            write["enableConverters"] = enableConverters;
            write["enableDidYouMean"] = enableDidYouMean;
            write["separateThousands"] = separateThousands;
            write["precision"] = precision;
            write["inputSign"] = inputSign;
            write["logEnabled"] = logEnabled;
            write["logFilePath"] = logFilePath;
            write["logTimeFormat"] = logTimeFormat;

            file.write(write.dump(4));
        }
    }
};

auto getStyled(double num, const Options& options) {
    struct ThousandsSep : std::numpunct<char> {
        char do_thousands_sep() const { return '\''; }
        std::string do_grouping() const { return "\3"; }
    };

    std::stringstream ss;
    if (options.separateThousands)
        ss.imbue(std::locale(ss.getloc(), new ThousandsSep));

    ss << (num < 1e40 ? std::fixed : std::scientific)
       << std::setprecision(options.precision) << num;

    auto numStr = ss.str();

    auto dot = numStr.find('.');
    if (dot != std::string::npos) {
        while (numStr.back() == '0') numStr.pop_back();
        if (numStr.back() == '.') numStr.pop_back();
    }

    return numStr;
}

auto createDefaultSession(my::File& file) {
    file.create();
    file.write(R"({
    "version": "0.2.0",
    "variables" : {}
})");
}

auto readVariables(const Options& options) {
    if (!options.enableVariables)
        return std::map<std::string, double>{};

    my::File file(SESSION_FILE);

    if (!file.exists()) {
        createDefaultSession(file);
        return std::map<std::string, double>{};
    }

    std::string buffer;
    file.read(buffer);
    const auto read = nlohmann::json::parse(buffer);

    return read.at("variables").get<std::map<std::string, double>>();
}

auto saveVariables(const std::map<std::string, double>& variables, const Options& options) {
    if (!options.enableVariables) return;
    my::File file(SESSION_FILE);

    if (!file.exists()) createDefaultSession(file);

    std::string buffer;
    file.read(buffer);
    auto read = nlohmann::json::parse(buffer);

    read["variables"].merge_patch(variables);

    file.clear();
    file.write(read.dump(4));
}

auto removeVariable(const std::string& variable, const Options& options) {
    if (!options.enableVariables) return;
    my::File file(SESSION_FILE);

    if (!file.exists()) createDefaultSession(file);

    std::string buffer;
    file.read(buffer);
    auto read = nlohmann::json::parse(buffer);

    read["variables"].erase(variable);

    file.clear();
    file.write(read.dump(4));
}

auto clearVariables(const Options& options) {
    if (!options.enableVariables) return;
    my::File file(SESSION_FILE);

    if (!file.exists()) createDefaultSession(file);

    std::string buffer;
    file.read(buffer);
    auto read = nlohmann::json::parse(buffer);

    read["variables"].clear();

    file.clear();
    file.write(read.dump(4));
}

template <class T>
auto logToFile(my::File& file, Options& options,
               const std::string& buffer, const T& res) {
    if (options.logEnabled) {
        my::setcol(std::cerr, my::Color::Red);  // to make all file error messages red

        if (!file.writef("{} > {}\n",
                         my::File::generateTimeStamp(options.logTimeFormat),
                         buffer)) {
            options.logEnabled = false;
            my::resetcol(std::cerr);
            return;
        }

        file.writef("{} :: {}\n\n",
                    my::File::generateTimeStamp(options.logTimeFormat),
                    res);

        my::resetcol(std::cerr);
    }
}

auto printHelp(const Options& options) {
    my::printcol(R"(
        [#f0b000:The list of supported operators:]
        +, -, /, *, % (modulus), ^ or ** (power), ! (factorial);

        [#f0b000:The list of supported functions:]
            [#f0b000:>] unary: sqrt, cbrt, 
                     ln, lg, exp, 
                     sin,   cos,   tan,   ctan, 
                     asin,  acos,  atan,  actan, 
                     sinh,  cosh,  tanh,  ctanh,
                     asinh, acosh, atanh, actanh,
                     sinc, fact, abs, ceil, floor, round, trunc
            [#f0b000:>] binary: log, min, max, gcd, lcm

        [#f0b000:The list of supported constants:] 
            [#f0b000:>] pi:  3.1415926535897932384
            [#f0b000:>] tau: 6.2831853071795862319
            [#f0b000:>] e:   2.7182818284590452354
            [#f0b000:>] phi: 1.6180339887498948482

        [#f0b000:The list of supported generators:] 
            [#f0b000:>] rnd: random number [0, 1]
            [#f0b000:>] time: time in milliseconds since epoch

        [#f0b000:!Usage example:] 
            sin(max(10 ** 2 - 4, 56) * -1) * (9! * 0.001) % 255

)");
    if (options.enableConverters)
        my::printcol(R"(
        [#f0b000:Parsing tree of supported converters:]
            [bin|oct|dec|hex][":"|" "][bin|oct|dec|hex][number]

        [#f0b000:!Usage example:] 
            dec:bin 342       = 101010110
            hex:dec 156       = 342
            bin:oct 101010110 = 526

)");
    if (options.enableVariables)
        my::printcol(R"(
        [#f0b000:!Variales:] 
            [#f0b000:>] To assign expression to variable use ()
                Example: x = (9! * 0.001)
                         x = (x * 42)
            [#f0b000:>] To checkout variables table: type vars
            [#f0b000:>] To clear variables: type cl vars
            [#f0b000:>] To remove variable: type rm name

)");
    my::printcol(R"(
        [#0e8bcf:*Note:] [#878787:log(base, number), gcd(greatest common divisor), lcm(least common multiple)
        To clear screen: type cls or clear(to clear even help message)
        To exit: type exit
        To get help: type help
        To enable log: type enable log
        To disable log: type disable log]

)");
}

auto main() -> int {
    SET_CONSOLE_VT_MODE();
    SET_UTF8_CONSOLE_CP();

    Options options{};

    const auto welcomeBanner = R"([#f0b000:
                 ┌─────────────────────────────────┐
                 │ Welcome to KorowaCalculator 1.5 |
                 └─────────────────────────────────┘]

                          /|                        /|
                          | \           __ _ _     / ;
                    ___    \ \   _.-"-" `~"\  `"--' /
                _.-'   ""-._\ ""   ._,"  ; "\"--._./
            _.-'       \./    "-""", )  ~"  |
           / ,- .'          ,    '  `(    ;  )
           \ ;/       '                 ;   /
            |/        '      |      \   '   |
            /        |             .."\  ,  |
           "         :       \   .'  : | ,. _)
           |         |     /     / |  |`--"--'
            \_        \    \    / _/  |
             \ "-._  _.|   (    /; -'/
              \  | "/  (   |   /,    |
               | \  |  /\  |\_///   /
               \ /   \ | \  \  /   /
                ||    \ \|  |  |  |
                ||     \ \  |  | /
                |\      |_|/   ||
                L_\       ||   ||
                          |\   |\
                          ( \. \ `.
                          |_ _\|_ _\
                            
)";

    const auto exitBanner = R"([#f0b000:
                 ┌───────────────────────────────────────────┐
                 │ Thanks for using KorowaCalculator 1.5     │
                 │ Have a nice day!                          │
                 │ Error reports goes here: @bitwise-demon   │
                 │ For poor programmer: 4149439315494553     │
                 └───────────────────────────────────────────┘]

                           _(__)_        V
                          '-e e -'__,--.__)
                           (o_o)        ) 
                              \. /___.  |
                              ||| _)/_)/
                             //_(/_(/_(

        press any key to exit...
)";

    const std::vector<std::string> tokens{
        "sqrt", "cbrt",
        "ln", "lg", "exp",
        "sin", "cos", "tan", "ctan", "asin", "acos", "atan", "actan",
        "sinh", "cosh", "tanh", "ctanh", "asinh", "acosh", "atanh", "actanh",
        "fact", "abs", "ceil", "floor", "round", "trunc",
        //
        "log", "min", "max", "gcd", "lcm",
        //
        "bin", "oct", "dec", "hex",
        //
        "pi", "phi", "tau", "e", "rnd",
        //
        "enable log", "disable log",
        "help", "exit", "cls", "clear", "vars", "cl vars"};

    if (options.showWelcomeScreen)
        my::printcol(welcomeBanner);

    if (options.alwaysShowHelp) printHelp(options);

    my::File file;
    if (options.logEnabled) {
        my::setcol(std::cerr, my::Color::Red);

        file.set(options.logFilePath + my::File::generateTimeStamp(
                                           "korowa (%d.%m.%Y - %H.%M.%S).log"));

        options.logEnabled = !!file.create();
        my::resetcol(std::cerr);
    }

    auto variables = readVariables(options);

    std::string buffer;

    for (;;) {
        size_t prevVarsSize = variables.size();

        my::printcol(options.inputSign.c_str());
        std::getline(std::cin, buffer);
        my::trim(buffer);

        // commands
        if (buffer.empty()) continue;

        if (buffer == "exit") break;

        if (buffer == "help") {
            printHelp(options);
            continue;
        }

        if (buffer == "cls" or buffer == "clear") {
            system("cls");
            if (options.alwaysShowHelp) printHelp(options);
            continue;
        }

        if (buffer == "vars" or buffer == "variables") {
            if (!options.enableVariables) {
                my::printcol("[#orange:Variables disabled in config file]\n\n");
                continue;
            }
            my::table(variables, {{4, 0}}, {"(name)", "(value)"});
            continue;
        }

        if (buffer == "cl vars" or buffer == "clear variables") {
            if (!options.enableVariables) {
                my::printcol("[#orange:Variables disabled in config file]\n\n");
                continue;
            }
            variables.clear();
            clearVariables(options);
            my::printcol("[#orange:Variables: cleared]\n\n");
            continue;
        }

        if (buffer.rfind("rm", 0) == 0) {
            if (!options.enableVariables) {
                my::printcol("[#orange:Variables disabled in config file]\n\n");
                continue;
            }

            auto tmp = buffer.substr(3);
            const auto name = my::trim(tmp);

            if (auto it = variables.find(name); it != variables.end()) {
                my::printcol("[#orange:Variable [{} = {}] removed]\n\n",
                             it->first, it->second);
                removeVariable(it->first, options);
                variables.erase(it);
                continue;
            }

            my::printcol("[#orange:Variable with name {} not found]\n\n", name);
            continue;
        }

        // commands

        // conversion routine
        if (options.enableConverters) {
            auto convertError = korowa::SyntaxError();
            const auto converted = korowa::convert(buffer, convertError);  // <- small subroutine for converting between different systems

            if (convertError.type() != korowa::SyntaxError::Type::Parsing) {
                if (convertError) {
                    my::printcol("[#red:Error occurred: \"{}\"\n\n]", convertError);
                    logToFile(file, options, buffer,
                              my::format("Error occurred: \"{}\"", convertError));
                    continue;
                }

                my::printf(0xcf760a, ":: {}\n\n", converted);
                logToFile(file, options, buffer, converted);

                continue;
            }
        }
        // conversion routine

        // eval routine
        auto evalError = korowa::SyntaxError();
        double result{};

        if (options.enableVariables)
            result = korowa::eval(buffer, evalError, variables);  // <- here all hot stuff happens
        else
            result = korowa::eval(buffer, evalError);

        if (evalError) {
            my::printcol("[#red:Error occurred: \"{}\"\n\n]", evalError);
            logToFile(file, options, buffer,
                      my::format("Error occurred: \"{}\"", evalError));

            if (options.enableDidYouMean) {
                if (evalError.type() == korowa::SyntaxError::Type::UnknownToken) {
                    auto meant = my::didYouMean(evalError.params().front(), tokens);
                    my::printcol("Did you mean: [#orange:{}]?\n\n", meant);
                }
            }
            continue;
        }

        const auto res = getStyled(result, options);
        my::printf(0x71db00, ":: {}\n\n", res);
        logToFile(file, options, buffer, res);

        // eval routine

        if (variables.size() != prevVarsSize) saveVariables(variables, options);
    }

    my::printcol(exitBanner);
    system("pause > nul");

    return 0;
}