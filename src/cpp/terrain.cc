// Copyright (c) 2015, Tamas Csala

#include "./terrain.h"
#include <string>

#include "engine/scene.h"

Terrain::Terrain(engine::GameObject* parent)
    : engine::GameObject(parent)
    , mesh_(scene_->shader_manager())
    , prog_(scene_->shader_manager()->get("terrain.vert"),
            //scene_->shader_manager()->get("terrain.geom"),
            scene_->shader_manager()->get("terrain.frag"))
    , uProjectionMatrix_(prog_, "uProjectionMatrix")
    , uCameraMatrix_(prog_, "uCameraMatrix")
    , uModelMatrix_(prog_, "uModelMatrix") {
  gl::Use(prog_);
  mesh_.setup(prog_, 0, 1);
  gl::UniformSampler(prog_, "uDiffuseTexture").set(2);
  gl::Bind(diffuseTexture_);
  // no alpha channel here
  diffuseTexture_.loadTexture("src/resources/textures/diffuse1.jpg", "CSRGB");
  diffuseTexture_.generateMipmap();
  diffuseTexture_.maxAnisotropy();
  diffuseTexture_.minFilter(gl::kLinearMipmapLinear);
  diffuseTexture_.magFilter(gl::kLinear);
  diffuseTexture_.wrapS(gl::kRepeat);
  diffuseTexture_.wrapT(gl::kRepeat);

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


