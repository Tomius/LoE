#include <Magick++.h>
#include <climits>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>

ushort foo(short texel) {
  if (texel < 0) {
    return (-texel)*2;
  } else {
    return texel*2;
  }
}

void fail(const std::string& src) {
  std::cout << "Error processing " << src << std::endl;
  std::terminate();
}

std::string destFilePath(std::string src) {
  const size_t last_slash_idx = src.find_last_of("\\/");
  if (std::string::npos != last_slash_idx) {
    src.erase(0, last_slash_idx + 1);
  }

  int x, y; char sx, sy;
  if (sscanf(src.c_str(), "%d%c%d%c_20101117_gmted_dsc300.tif", &y, &sy, &x, &sx) != 4) {
    fail(src);
  }
  if (sy == 'n' || sy == 'N') {
    y = 90 - y;
  } else if (sy == 's' || sy == 'S') {
    y = y + 90;
  } else {
    fail(src);
  }
  // There's no info about the north pole.
  if (y == 20) {
    y = 0;
  }

  if (sx == 'e' || sx == 'E') {
    x = 180 - x;
  } else if (sx == 'w' || sx == 'W') {
    x = x + 180;
  } else {
    fail(src);
  }

  std::stringstream ss;
  ss << "terrain/" << std::setw(3) << std::setfill('0') << x
     << '_' << std::setw(3) << std::setfill('0') << y << ".jpg";
  return ss.str();
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
  size_t width = geom.width(), height = geom.height();
  ushort *data = new ushort[3 * geom.width() * geom.height()];
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      data[3*(y*width +x)+0] = foo(image.pixelColor(x, y).redQuantum());
      data[3*(y*width +x)+1] = foo(image.pixelColor(x, y).greenQuantum());
      data[3*(y*width +x)+2] = foo(image.pixelColor(x, y).blueQuantum());
    }
  }
  Magick::Image image0{width, height, "RGB", Magick::StorageType::ShortPixel, data};
  image0.quantizeColorSpace(Magick::GRAYColorspace);
  image0.write(dest);
}
