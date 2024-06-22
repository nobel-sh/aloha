#include <iostream>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
void print(const char *str) { std::cout << str; }

void println(const char *str) { std::cout << str << std::endl; }

void printNum(double value) { std::cout << value; }

void printlnNum(double value) { std::cout << value << std::endl; }

char *input() {
  char *in = (char *)malloc(100 * sizeof(char));
  if (in == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  if (scanf("%99s", in) != 1) {
    fprintf(stderr, "Could not read stdin input\n");
    free(in);
    exit(1);
  }

  return in;
}
}
