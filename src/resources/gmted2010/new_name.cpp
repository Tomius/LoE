#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>

int main(int argc, char** argv) {
  assert(argc > 1);
  int x, y;
  std::string input = argv[1];
  std::string dir = "";
  const size_t last_slash_idx = input.find_last_of("\\/");
  if (last_slash_idx != std::string::npos) {
    dir = input.substr(0, last_slash_idx + 1);
    input.erase(0, last_slash_idx + 1);
  }
  sscanf(input.c_str(), "%d_%d.jpg", &x, &y);
  printf((dir + "%03d_%03d.jpg").c_str(), 360-x, y ? y-20 : y);
  return 0;
}
