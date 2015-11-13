// Copyright (c) 2015, Tamas Csala

#include "./scattering.h"
#include "engine/game_engine.h"
#include "engine/global_height_map.h"

Scattering::Scattering(engine::GameObject* parent)
    : engine::Behaviour(parent)
    , rect_({gl::RectangleShape::kPosition})
    , prog_(scene_->shader_manager()->get("rectangle.vert"),
            scene_->shader_manager()->get("scattering.frag"))
    , uZFar_(prog_, "uZFar")
    , uResolution_(prog_, "uResolution")
    , uCamPos_(prog_, "uCamPos")
    , uCameraMatrix_(prog_, "uCameraMatrix") {

  gl::Use(prog_);
  gl::UniformSampler(prog_, "uTex").set(0);
  gl::UniformSampler(prog_, "uDepthTex").set(1);
  gl::Uniform<glm::ivec2>(prog_, "uTexSize") =
    glm::ivec2(engine::GlobalHeightMap::tex_w, engine::GlobalHeightMap::tex_h);
  (prog_ | "aPosition").bindLocation(rect_.kPosition);
  prog_.validate();

  gl::Bind(color_tex_);
  color_tex_.upload(gl::kRgb16F, 1, 1, gl::kRgb, gl::kFloat, nullptr);
  color_tex_.minFilter(gl::kLinearMipmapLinear);
  color_tex_.magFilter(gl::kLinear);
  gl::Unbind(color_tex_);

  gl::Bind(depth_tex_);
  depth_tex_.upload(gl::kR32F, 1, 1, gl::kRed, gl::kFloat, nullptr);
  depth_tex_.minFilter(gl::kLinear);
  depth_tex_.magFilter(gl::kLinear);
  gl::Unbind(depth_tex_);

  gl::Bind(depth_buffer_);
  depth_buffer_.storage(gl::kDepthComponent, 1, 1);
  gl::Unbind(depth_buffer_);

  gl::Bind(fbo_);
  fbo_.attachTexture(gl::kColorAttachment0, color_tex_);
  fbo_.attachTexture(gl::kColorAttachment1, depth_tex_);
  fbo_.attachBuffer(gl::kDepthAttachment, depth_buffer_);

  GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, attachments);

  fbo_.validate();
  gl::Unbind(fbo_);
}

void Scattering::screenResized(size_t w, size_t h) {
  gl::Use(prog_);
  uResolution_ = glm::vec2(w, h);

  gl::Bind(color_tex_);
  color_tex_.upload(gl::kRgb16F, w, h, gl::kRgb, gl::kFloat, nullptr);
  gl::Unbind(color_tex_);

  gl::Bind(depth_tex_);
  depth_tex_.upload(gl::kR32F, w, h, gl::kRed, gl::kFloat, nullptr);
  gl::Unbind(depth_tex_);

  gl::Bind(depth_buffer_);
  depth_buffer_.storage(gl::kDepthComponent, w, h);
  gl::Unbind(depth_buffer_);
}

void Scattering::update() {
  gl::Bind(fbo_);
  gl::Clear().Depth(); // the sky.frag will clear the color attachments
}

void Scattering::render2D() {
  gl::Unbind(gl::kFramebuffer);

  gl::BindToTexUnit(color_tex_, 0);
  color_tex_.generateMipmap();
  gl::BindToTexUnit(depth_tex_, 1);

  gl::Use(prog_);
  prog_.update();

  auto cam = scene_->camera();
  uCameraMatrix_ = glm::mat3(cam->cameraMatrix());
  uCamPos_ = cam->transform()->pos();
  uZFar_ = cam->z_far();

  rect_.render();

  gl::UnbindFromTexUnit(depth_tex_, 1);
  gl::UnbindFromTexUnit(color_tex_, 0);
}
