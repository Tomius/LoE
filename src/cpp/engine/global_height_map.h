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
  static size_t w, h;
  static size_t atlas_w, atlas_h;
  static size_t atlas_elem_count;
  static size_t atlas_elem_w;
  static size_t atlas_elem_h;
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

  static void upload(gl::Texture2DArray& tex) {
    int w = atlas_elem_w;
    int h = atlas_elem_h;
    int d = atlas_elem_count;
    int levels = 1 + std::floor(std::log2(std::max(w, h)));
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, GL_R8, w, h, d);

    int level = 0;
    for (int y = 0; y < 180; y += 20) {
      for (int x = 0; x < 360; x += 30, ++level) {
        char filename[12];
        sprintf(filename, "%03d_%03d.jpg", x, y);

        std::string path = std::string{base_path} + filename;
        Magick::Image image(path);
        size_t w = image.columns();
        size_t h = image.rows();
        GLubyte *data = new GLubyte[w*h];
        image.write(0, 0, w, h, "R", MagickCore::CharPixel, data);

        bool bad_alignment = w % 4 != 0;
        GLint unpack_aligment;

        if (bad_alignment) {
          glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_aligment);
          gl::PixelStore(gl::kUnpackAlignment, 1);
        }

        tex.subUpload(0, 0, level, w, h, 1, gl::kRed, gl::kUnsignedByte, data);

        if (bad_alignment) {
          gl::PixelStore(gl::kUnpackAlignment, unpack_aligment);
        }

        delete[] data;
      }
    }
    tex.generateMipmap();
  }
};

}  // namespace engine

#endif
