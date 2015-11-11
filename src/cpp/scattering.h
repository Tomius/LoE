// Copyright (c) 2015, Tamas Csala

#ifndef LOE_SCATTERING_H_
#define LOE_SCATTERING_H_

#include "engine/oglwrap_all.h"
#include "engine/scene.h"
#include "engine/behaviour.h"
#include "./skybox.h"

class Scattering : public engine::Behaviour {
 public:
  explicit Scattering(engine::GameObject* parent);
  virtual ~Scattering() {}

  glm::vec3 getSunPos() const;
  glm::vec3 getLightSourcePos() const;

  virtual void screenResized(size_t w, size_t h) override;
  virtual void update() override;
  virtual void render2D() override;

 private:
  gl::RectangleShape rect_;
  engine::ShaderProgram prog_;
  gl::Framebuffer fbo_;
  gl::Texture2D color_tex_, depth_tex_;
  gl::LazyUniform<float> uZNear_, uZFar_;
  gl::LazyUniform<glm::vec2> uResolution_;
  gl::LazyUniform<glm::mat3> uCameraMatrix_;
};


#endif  // LOE_SCATTERING_H_
