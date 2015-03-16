// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_HEIGHT_MAP_INTERFACE_H_
#define ENGINE_HEIGHT_MAP_INTERFACE_H_

#include "./oglwrap_all.h"

namespace engine {

// An interface to get data from a heightmap
class HeightMapInterface {
 public:
  virtual ~HeightMapInterface() {}

  // The width and height of the texture (not the size of the heightmap)
  virtual int w() const = 0;
  virtual int h() const = 0;

  virtual glm::vec2 extent() const = 0;
  virtual glm::vec2 center() const = 0;

  // Returns if the texture coordinates are valid
  virtual bool valid(double x, double z) const = 0;

  // Texture space fetch
  virtual double heightAt(int s, int t) const = 0;

  // Texture space fetch with interpolation
  virtual double heightAt(double s, double t) const = 0;

  // Returns the format of the height data
  virtual gl::PixelDataFormat format() const = 0;

  // Returns the type of the height data
  virtual gl::PixelDataType type() const = 0;

  // Uploads the heightmap to a texture object
  virtual void upload(gl::Texture2D& tex) const {};
  virtual void upload(gl::Texture2DArray& tex) const {};

  // Returns a pointer to the heightfield data
  virtual const void* data() const = 0;
};

}  // namespace engine

#endif
