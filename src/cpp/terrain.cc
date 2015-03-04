// Copyright (c) 2014, Tamas Csala

#include "./terrain.h"
#include <string>

#include "engine/scene.h"

Terrain::Terrain(engine::GameObject* parent)
    : engine::GameObject(parent)
    , mesh_(scene_->shader_manager(), height_map_)
    , prog_(scene_->shader_manager()->get("terrain.vert"),
            scene_->shader_manager()->get("terrain.frag"))
    , uProjectionMatrix_(prog_, "uProjectionMatrix")
    , uCameraMatrix_(prog_, "uCameraMatrix")
    , uModelMatrix_(prog_, "uModelMatrix") {
  gl::Use(prog_);
  mesh_.setup(prog_, 1);
  gl::UniformSampler(prog_, "uGrassMap0").set(2);
  gl::UniformSampler(prog_, "uGrassMap1").set(3);
  for (int i = 0; i < 2; ++i) {
    gl::Bind(grassMaps_[i]);
    // no alpha channel here
    grassMaps_[i].loadTexture(std::string{"src/resources/textures/"} +
        (i == 0 ? "grass.jpg" : "grass_2.jpg"), "CSRGB");
    grassMaps_[i].generateMipmap();
    grassMaps_[i].maxAnisotropy();
    grassMaps_[i].minFilter(gl::kLinearMipmapLinear);
    grassMaps_[i].magFilter(gl::kLinear);
    grassMaps_[i].wrapS(gl::kRepeat);
    grassMaps_[i].wrapT(gl::kRepeat);
  }

  gl::UniformSampler(prog_, "uGrassNormalMap").set(4);
  gl::Bind(grassNormalMap_);
  {
    // the normal map doesn't have an alpha channel, and is not is srgb space
    grassNormalMap_.loadTexture("src/resources/textures/grass_normal.jpg", "CRGB");
    grassNormalMap_.generateMipmap();
    grassNormalMap_.minFilter(gl::kLinearMipmapLinear);
    grassNormalMap_.magFilter(gl::kLinear);
    grassNormalMap_.wrapS(gl::kRepeat);
    grassNormalMap_.wrapT(gl::kRepeat);
  }

  prog_.validate();
}

void Terrain::render() {
  const engine::Camera& cam = *scene_->camera();

  gl::Use(prog_);
  prog_.update();
  uCameraMatrix_ = cam.cameraMatrix();
  uProjectionMatrix_ = cam.projectionMatrix();
  uModelMatrix_ = transform()->matrix();

  gl::BindToTexUnit(grassMaps_[0], 2);
  gl::BindToTexUnit(grassMaps_[1], 3);
  gl::BindToTexUnit(grassNormalMap_, 4);
  mesh_.render(cam);

  gl::UnbindFromTexUnit(grassNormalMap_, 4);
  gl::UnbindFromTexUnit(grassMaps_[1], 3);
  gl::UnbindFromTexUnit(grassMaps_[0], 2);
}


