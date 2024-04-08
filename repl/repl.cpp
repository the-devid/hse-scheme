#include "../src/scheme.h"

#include <exception>
#include <iostream>

int main() {
    Interpreter interpreter;
    std::string s;
    std::cout << "> ";
    while (std::getline(std::cin, s)) {
        try {
            std::cout << interpreter.Run(s) << std::endl;
        } catch (std::exception& e) {
            std::cerr << "[ERROR]: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR]: Some error occured" << std::endl;
        }
        std::cout << "> ";
    }
}
