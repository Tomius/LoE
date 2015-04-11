// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TERRAIN_MESH_H_
#define ENGINE_CDLOD_TERRAIN_MESH_H_

#include "./quad_tree.h"
#include "./texture/tex_quad_tree.h"
#include "../oglwrap_all.h"
#include "../shader_manager.h"

namespace engine {
namespace cdlod {

class TerrainMesh {
 public:
  explicit TerrainMesh(engine::ShaderManager* manager);
  void setup(const gl::Program& program, int tex_unit, int index_tex_unit);
  void render(const Camera& cam);

 private:
  QuadTree quad_tree_;
  TexQuadTree tex_quad_tree_;
  std::unique_ptr<gl::LazyUniform<glm::vec3>> uCamPos_;
  std::unique_ptr<gl::LazyUniform<GLfloat>> uNodeDimension_;
  int tex_unit_, index_tex_unit_;
};

}  // namespace cdlod

}  // namespace engine

#endif
