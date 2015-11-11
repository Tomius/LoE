// Copyright (c) 2015, Tamas Csala

#ifndef LOE_SCATTERING_H_
#define LOE_SCATTERING_H_

#include "engine/oglwrap_all.h"
#include "engine/scene.h"
#include "engine/behaviour.h"

class Scattering : public engine::Behaviour {
 public:
  explicit Scattering(engine::GameObject* parent);
  virtual ~Scattering() {}

  glm::vec3 getSunPos() const;
  glm::vec3 getLightSourcePos() const;

  virtual void render2D() override;

 private:
  gl::RectangleShape rect_;
  engine::ShaderProgram prog_;

  gl::LazyUniform<glm::mat3> uCameraMatrix_;
};


#endif  // LOE_SCATTERING_H_
