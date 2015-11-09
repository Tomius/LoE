// Copyright (c) 2015, Tamas Csala

#include "./skybox.h"
#include "engine/game_engine.h"
#include "engine/global_height_map.h"

const float day_duration = 512.0f, day_start = 0;

Skybox::Skybox(engine::GameObject* parent)
    : engine::Behaviour(parent)
    , time_(day_start)
    , cube_({gl::CubeShape::kPosition})
    , prog_(scene_->shader_manager()->get("skybox.vert"),
            scene_->shader_manager()->get("skybox.frag"))
    , uProjectionMatrix_(prog_, "uProjectionMatrix")
    , uCameraMatrix_(prog_, "uCameraMatrix") {
  engine::ShaderFile *sky_fs = scene_->shader_manager()->get("sky.frag");
  sky_fs->set_update_func([this](const gl::Program& prog) {
    gl::Uniform<glm::vec3>(prog, "uSunPos") = getSunPos();
  });

  gl::Use(prog_);
  prog_.validate();
  (prog_ | "aPosition").bindLocation(cube_.kPosition);
}

glm::vec3 Skybox::getSunPos() const {
  return glm::normalize(
           glm::vec3{-cos(time_ * 2 * M_PI / day_duration), 0.3f,
                     -sin(time_ * 2 * M_PI / day_duration)});
}

glm::vec3 Skybox::getLightSourcePos() const {
  glm::vec3 sun_pos = getSunPos();
  return sun_pos.y > 0 ? sun_pos : -sun_pos;
}

void Skybox::update() {
  time_ += scene_->environment_time().dt * mult_;
}

void Skybox::keyAction(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_KP_ADD) {
      mult_ = 64.0;
    } else if (key == GLFW_KEY_KP_SUBTRACT) {
      mult_ = -64.0;
    }
  } else if (action == GLFW_RELEASE) {
    if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_KP_SUBTRACT) {
      mult_ = 1.0;
    }
  }
}

void Skybox::render() {
  auto cam = scene_->camera();

  gl::Use(prog_);
  prog_.update();
  uCameraMatrix_ = glm::mat3(cam->cameraMatrix());
  uProjectionMatrix_ = cam->projectionMatrix();
  auto camera = scene_->camera();
  float scale = (camera->z_near() + camera->z_far()) / 2;
  gl::Uniform<float>(prog_, "uScale") = scale;
  gl::Uniform<glm::vec3>(prog_, "uCamPos") = cam->transform()->pos();
  gl::Uniform<glm::ivec2>(prog_, "uResolution") =
    glm::ivec2(engine::GameEngine::window_size());
  gl::Uniform<glm::ivec2>(prog_, "uTexSize") =
    glm::ivec2(engine::GlobalHeightMap::tex_w, engine::GlobalHeightMap::tex_h);

  gl::TemporaryDisable depth_test{gl::kDepthTest};

  gl::DepthMask(false);
  cube_.render();
  gl::DepthMask(true);
}
