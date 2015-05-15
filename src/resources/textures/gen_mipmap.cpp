#include <Magick++.h>
#include <climits>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <cassert>

bool file_exists(const char *fileName) {
    return std::ifstream{fileName}.good();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("Add meg parancssori parameterkent, hogy melyik fajlt dolgozzam fel!");
    return 1;
  } else if (!file_exists(argv[1])) {
    puts("A megadott fajl nem letezik!");
    return 1;
  }

  Magick::Image image{argv[1]};
  Magick::Geometry geom = image.size();
  size_t w = geom.width(), h = geom.height();
  size_t level = 0;
  while (w > 1 || h > 1) {
    w = std::max<size_t>(1, w/2);
    h = std::max<size_t>(1, h/2);
    level++;

    image.resize(std::to_string(w) + "x" + std::to_string(h) + "!");
    assert(w == image.columns() && h == image.rows());
    image.write(std::to_string(level) + "/" + argv[1]);
  }

  return 0;
}
