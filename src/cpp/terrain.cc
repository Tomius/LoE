// Copyright (c) 2015, Tamas Csala

#include "./terrain.h"
#include <string>

#include "engine/scene.h"

static bool file_exists(const char *fileName) {
    return std::ifstream{fileName}.good();
}

Terrain::Terrain(engine::GameObject* parent)
    : engine::GameObject(parent)
    , mesh_(scene_->shader_manager())
    , prog_(scene_->shader_manager()->get("terrain.vert"),
            scene_->shader_manager()->get("terrain.frag"))
    , uProjectionMatrix_(prog_, "uProjectionMatrix")
    , uCameraMatrix_(prog_, "uCameraMatrix")
    , uModelMatrix_(prog_, "uModelMatrix") {
  gl::Use(prog_);
  mesh_.setup(prog_, 0, 1);
  gl::UniformSampler(prog_, "uDiffuseTexture").set(2);
  gl::Bind(diffuseTexture_);

  size_t level = 0;
  std::string dir = "src/resources/textures/" + std::to_string(level);
  std::string textures[16];
  while (file_exists(dir.c_str())) {
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        textures[4*y+x] = dir + "/earth" + std::to_string(x)
                          + std::to_string(y) + ".png";
      }
    }
    diffuseTexture_.loadTextures(std::begin(textures), std::end(textures),
                                 "CSRGB", level); // no alpha

    dir = "src/resources/textures/" + std::to_string(++level);
  }

  diffuseTexture_.maxAnisotropy();
  diffuseTexture_.minFilter(gl::kLinearMipmapLinear);
  diffuseTexture_.magFilter(gl::kLinear);
  diffuseTexture_.wrapS(gl::kClampToEdge);
  diffuseTexture_.wrapT(gl::kClampToEdge);

  prog_.validate();
}

void Terrain::render() {
  const engine::Camera& cam = *scene_->camera();

  gl::Use(prog_);
  prog_.update();
  uCameraMatrix_ = cam.cameraMatrix();
  uProjectionMatrix_ = cam.projectionMatrix();
  uModelMatrix_ = transform()->matrix();

  gl::BindToTexUnit(diffuseTexture_, 2);
  mesh_.render(cam);
  gl::UnbindFromTexUnit(diffuseTexture_, 2);
}


