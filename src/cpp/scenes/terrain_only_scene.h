#ifndef TERRAIN_ONLY_SCENE_H_
#define TERRAIN_ONLY_SCENE_H_

#include "../engine/misc.h"
#include "../engine/scene.h"
#include "../engine/camera.h"
#include "../engine/behaviour.h"
#include "../engine/debug/debug_shape.h"
#include "../engine/gui/label.h"
#include "../loading_screen.h"
#include "../skybox.h"
#include "../terrain.h"
#include "../fps_display.h"
#include "../after_effects.h"

class TerrainOnlyScene : public engine::Scene {
 public:
  TerrainOnlyScene() {
    #if !ENGINE_NO_FULLSCREEN
      glfwSetInputMode(window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    #endif

    LoadingScreen().render();
    glfwSwapBuffers(window());

    auto skybox = addComponent<Skybox>();
    skybox->set_group(-1);
    addComponent<Terrain>();
    camera_ = addComponent<engine::FreeFlyCamera>(M_PI/3, 2, 150000,
        glm::vec3(5000, 5000, 5000), glm::vec3(0, 0, 0), 500, 2);
    //camera_->transform()->set_up({0, -1, 0});
    set_camera(camera_);
    auto after_effects = addComponent<AfterEffects>(skybox);
    after_effects->set_group(1);
    auto fps = addComponent<FpsDisplay>();
    fps->set_group(2);
  }

 private:
  engine::FreeFlyCamera* camera_;

  virtual void mouseScrolled(double xoffset, double yoffset) override {
    camera_->set_speed_per_sec(camera_->speed_per_sec() * (yoffset > 0 ? 1.1 : 0.9));
  }
};

#endif
