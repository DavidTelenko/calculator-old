#include <cmath>
#include <iostream>
#include <my/extention/File.hpp>
#include <my/extention/Math.hpp>
#include <my/extention/NumParse.hpp>

auto main() -> int {
    my::File file;
    file.set("../folder/File.txt");
    file.create();

    // std::cout << std::stod("1.") << std::endl;
    // std::cout << std::sinh(1.0) << std::endl;
    // std::cout << (std::sin(12.0) / 12.0) << std::endl;
    // std::cout << (std::sin(12.0 * my::PI) / (12.0 * my::PI)) << std::endl;
    // std::cout << my::sinc(12.0) << std::endl;
    // std::cout << my::parse<double>("0.2") << std::endl;
    // std::cout << my::parse<double>(".2") << std::endl;
    //-------------do not write code after this line----------------
    system("pause > nul");
    return 0;
}