// Copyright (c) 2015, Tamas Csala

#ifndef LOE_TERRAIN_H_
#define LOE_TERRAIN_H_

#include "./skybox.h"
#include "./shadow.h"
#include "engine/oglwrap_all.h"
#include "engine/global_height_map.h"
#include "engine/game_object.h"
#include "engine/shader_manager.h"
#include "engine/cdlod/terrain_mesh.h"

class Terrain : public engine::GameObject {
 public:
  explicit Terrain(engine::GameObject* parent);
  virtual ~Terrain() {}

 private:
  engine::cdlod::TerrainMesh mesh_;
  engine::ShaderProgram prog_;  // has to be inited after mesh_

  gl::Texture2DArray diffuseTexture_;
  gl::LazyUniform<glm::mat4> uProjectionMatrix_, uCameraMatrix_, uModelMatrix_;

  virtual void render() override;
};

#endif  // LOE_TERRAIN_H_
