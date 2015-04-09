// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_SCENE_H_
#define ENGINE_SCENE_H_

#include <vector>
#include <memory>

#include "./timer.h"
#include "./camera.h"
#include "./behaviour.h"
#include "./game_object.h"
#include "./oglwrap_all.h"
#include "./shader_manager.h"

namespace engine {

class GameObject;

class Scene : public Behaviour {
 public:
  Scene();

  virtual float gravity() const { return 9.81f; }

  const Timer& game_time() const { return game_time_; }
  Timer& game_time() { return game_time_; }

  const Timer& environment_time() const { return environment_time_; }
  Timer& environment_time() { return environment_time_; }

  const Timer& camera_time() const { return camera_time_; }
  Timer& camera_time() { return camera_time_; }

  const Camera* camera() const { return camera_; }
  Camera* camera() { return camera_; }
  void set_camera(Camera* camera) { camera_ = camera; }

  ShaderManager* shader_manager();

  GLFWwindow* window() const { return window_; }
  void set_window(GLFWwindow* window) { window_ = window; }

  virtual void keyAction(int key, int scancode, int action, int mods) override {
    if (action == GLFW_PRESS) {
      switch (key) {
        case GLFW_KEY_F1:
          game_time_.toggle();
          break;
        case GLFW_KEY_F2:
          environment_time_.toggle();
          break;
        default:
          break;
      }
    }
  }

  virtual void turn() {
    updateAll();
    renderAll();
    render2DAll();
  }

 protected:
  Camera* camera_;
  Timer game_time_, environment_time_, camera_time_;
  GLFWwindow* window_;

  virtual void updateAll() override {
    game_time_.tick();
    environment_time_.tick();
    camera_time_.tick();

    Behaviour::updateAll();
  }

  virtual void renderAll() override {
    if (camera_) { Behaviour::renderAll(); }
  }

  virtual void render2DAll() override {
    gl::TemporarySet capabilities{{{gl::kBlend, true},
                                   {gl::kCullFace, false},
                                   {gl::kDepthTest, false}}};
    gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);

    Behaviour::render2DAll();
  }
};

}  // namespace engine


#endif
