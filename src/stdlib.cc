#include <iostream>

extern "C" {
void print(const char *str) { std::cout << str; }

void println(const char *str) { std::cout << str << std::endl; }

void printNum(double value) { std::cout << value; }

void printlnNum(double value) { std::cout << value << std::endl; }
}
