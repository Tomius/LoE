// Copyright (c) 2015, Tamas Csala

#include "./scene.h"
#include "./game_engine.h"

namespace engine {

Scene::Scene()
    : Behaviour(nullptr)
    , camera_(nullptr)
    , window_(GameEngine::window()) {
  set_scene(this);
}

ShaderManager* Scene::shader_manager() {
  return GameEngine::shader_manager();
}


}  // namespace engine
