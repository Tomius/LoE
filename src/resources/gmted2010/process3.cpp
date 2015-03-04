#include <Magick++.h>
#include <climits>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>

void fail(const std::string& src) {
  std::cout << "Error processing " << src << std::endl;
  std::terminate();
}

std::string destFilePath(std::string src) {
  src[0] += 1;
  return src;
}

ushort fetchRed(Magick::Image const& image, int x, int y) {
  return (((int)image.pixelColor(2*x, 2*y).redQuantum()) +
      image.pixelColor(2*x+1, 2*y).redQuantum() +
      image.pixelColor(2*x, 2*y+1).redQuantum() +
      image.pixelColor(2*x+1, 2*y+1).redQuantum()) / 4;
}

ushort fetchGreen(Magick::Image const& image, int x, int y) {
  return (((int)image.pixelColor(2*x, 2*y).greenQuantum()) +
      image.pixelColor(2*x+1, 2*y).greenQuantum() +
      image.pixelColor(2*x, 2*y+1).greenQuantum() +
      image.pixelColor(2*x+1, 2*y+1).greenQuantum()) / 4;
}

ushort fetchBlue(Magick::Image const& image, int x, int y) {
  return (((int)image.pixelColor(2*x, 2*y).blueQuantum()) +
      image.pixelColor(2*x+1, 2*y).blueQuantum() +
      image.pixelColor(2*x, 2*y+1).blueQuantum() +
      image.pixelColor(2*x+1, 2*y+1).blueQuantum()) / 4;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("Add meg parancssori parameterkent, hogy melyik fajlt dolgozzam fel!");
    return 1;
  }
  std::string dest{destFilePath(argv[1])};
  std::cout << "Creating " << dest << std::endl;
  Magick::Image image{argv[1]};
  Magick::Geometry geom = image.size();
  assert(geom.width() % 2 == 0 && geom.height() % 2 == 0);
  size_t width = geom.width()/2, height = geom.height()/2;
  ushort *data = new ushort[3 * width * height];
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      ushort red =
      data[3*(y*width+x)+0] = fetchRed(image, x, y);
      data[3*(y*width+x)+1] = fetchGreen(image, x, y);
      data[3*(y*width+x)+2] = fetchBlue(image, x, y);
    }
  }
  Magick::Image image0{width, height, "RGB", Magick::StorageType::ShortPixel, data};
  image0.quantizeColorSpace(Magick::GRAYColorspace);
  image0.write(dest);
}
