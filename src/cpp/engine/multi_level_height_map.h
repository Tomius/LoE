// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_MULTI_LEVEL_HEIGHT_MAP_H_
#define ENGINE_MULTI_LEVEL_HEIGHT_MAP_H_

#include <climits>
#include "./transform.h"
#include "./height_map_interface.h"
#include "./texture_source.h"
#include "./global_terrain_setting.h"

namespace engine {

class GlobalHeightMap : public HeightMapInterface {
  static constexpr size_t w_ = global_terrain::w, h_ = global_terrain::h;

 public:
  GlobalHeightMap() {}

  // The width and height of the texture
  virtual int w() const override { return w_; }
  virtual int h() const override { return h_; }

  virtual glm::vec2 extent() const override {
    return glm::vec2(w(), h());
  }

  virtual glm::vec2 center() const override {
    return extent()/2.0f;
  }

  virtual bool valid(double x, double y) const override {
    return 0 < x && x < w() && 0 < y && y < h();
  }

  virtual double heightAt(int s, int t) const override {
    throw new std::logic_error("Unimplemented function heightAt");
  }

  virtual double heightAt(double s, double t) const override {
    throw new std::logic_error("Unimplemented function heightAt");
  }

  virtual gl::PixelDataFormat format() const override {
    return gl::kRed;
  }

  virtual gl::PixelDataType type() const override {
    return gl::kUnsignedByte;
  }

  virtual void upload(gl::Texture2D& tex) const override {}

  virtual void upload(gl::Texture2DArray& tex) const override {
    std::string dir = global_terrain::base_path;
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8,
                   global_terrain::atlas_elem_w,
                   global_terrain::atlas_elem_h,
                   global_terrain::atlas_elem_num);
    int i = 0;
    for (int y = 0; y < 180; y += 20) {
      for (int x = 0; x < 360; x += 30, ++i) {
        char filename[12];
        sprintf(filename, "%03d_%03d.jpg", x, y);

        std::string path = dir + filename;
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

        tex.subUpload(0, 0, i, w, h, 1, gl::kRed, gl::kUnsignedByte, data);

        if (bad_alignment) {
          gl::PixelStore(gl::kUnpackAlignment, unpack_aligment);
        }

        delete[] data;
      }
    }
    tex.generateMipmap();
  }

  virtual const void* data() const override {
    throw new std::logic_error("data");
  }
};

}  // namespace engine

#endif
