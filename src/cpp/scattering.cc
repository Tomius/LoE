// Copyright (c) 2015, Tamas Csala

#include "./scattering.h"
#include "engine/game_engine.h"
#include "engine/global_height_map.h"

Scattering::Scattering(engine::GameObject* parent)
    : engine::Behaviour(parent)
    , rect_({gl::RectangleShape::kPosition})
    , prog_(scene_->shader_manager()->get("rectangle.vert"),
            scene_->shader_manager()->get("scattering.frag"))
    , uCameraMatrix_(prog_, "uCameraMatrix") {

  gl::Use(prog_);
  prog_.validate();
  (prog_ | "aPosition").bindLocation(rect_.kPosition);
}

void Scattering::render2D() {
  auto cam = scene_->camera();

  gl::Use(prog_);
  prog_.update();
  uCameraMatrix_ = glm::mat3(cam->cameraMatrix());
  gl::Uniform<glm::vec3>(prog_, "uCamPos") = cam->transform()->pos();
  gl::Uniform<glm::ivec2>(prog_, "uResolution") =
    glm::ivec2(engine::GameEngine::window_size());
  gl::Uniform<glm::ivec2>(prog_, "uTexSize") =
    glm::ivec2(engine::GlobalHeightMap::tex_w, engine::GlobalHeightMap::tex_h);

  gl::BlendFunc(gl::kSrcAlpha, gl::kOne);
  rect_.render();
  gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);
}
