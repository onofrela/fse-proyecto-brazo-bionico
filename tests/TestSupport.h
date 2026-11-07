#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H
#include <cstdlib>
#include <iostream>
#include <string>
// Cada caso publica evidencia consumible por scripts/test.py.
inline void result(const char *id, const char *expected, const std::string &obtained, bool passed) {
  std::cout << id << '|' << expected << '|' << obtained << '|' << (passed ? "PASA" : "FALLA") << '\n';
  if (!passed) std::exit(EXIT_FAILURE);
}
#endif
