// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_GLOBAL_HEGIHT_MAP_H_
#define ENGINE_GLOBAL_HEGIHT_MAP_H_

#include <climits>
#include "./transform.h"
#include "./texture_source.h"

namespace engine {

class GlobalHeightMap {
 public:
  static const char *base_path;
  //static size_t level_offset;
  static size_t w, h;
  static float sphere_radius;

  static glm::vec2 extent() {
    return glm::vec2(w, h);
  }

  static glm::vec2 center() {
    return extent()/2.0f;
  }

  static bool valid(double x, double y) {
    return 0 < x && x < w && 0 < y && y < h;
  }

  static gl::PixelDataFormat format() {
    return gl::kRed;
  }

  static gl::PixelDataType type() {
    return gl::kUnsignedByte;
  }
};

}  // namespace engine

#endif
