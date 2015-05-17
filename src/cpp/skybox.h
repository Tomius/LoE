// Copyright (c) 2015, Tamas Csala

#ifndef LOD_SKYBOX_H_
#define LOD_SKYBOX_H_

#include "engine/oglwrap_all.h"
#include "engine/scene.h"
#include "engine/behaviour.h"

class Skybox : public engine::Behaviour {
 public:
  explicit Skybox(engine::GameObject* parent);
  virtual ~Skybox() {}

  glm::vec3 getSunPos() const;
  glm::vec3 getLightSourcePos() const;

  virtual void render() override;
  virtual void update() override;
  virtual void keyAction(int key, int scancode, int action, int mods) override;

 private:
  float time_;
  float mult_ = 1.0;
  gl::CubeShape cube_;

  engine::ShaderProgram prog_;

  gl::LazyUniform<glm::mat4> uProjectionMatrix_;
  gl::LazyUniform<glm::mat3> uCameraMatrix_;
};


#endif  // LOD_SKYBOX_H_
