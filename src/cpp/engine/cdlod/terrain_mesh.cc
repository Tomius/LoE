// Copyright (c) 2015, Tamas Csala

#include "./terrain_mesh.h"
#include "../oglwrap_all.h"

#define gl(func) OGLWRAP_CHECKED_FUNCTION(func)

namespace engine {
namespace cdlod {

TerrainMesh::TerrainMesh(engine::ShaderManager* manager) {
  gl::ShaderSource vs_src{"engine/cdlod_terrain.vert"};
  manager->publish("engine/cdlod_terrain.vert", vs_src);
}

void TerrainMesh::setup(const gl::Program& program,
                        int tex_unit, int index_tex_unit) {
  gl::Use(program);

  quad_tree_.setupPositions(program | "CDLODTerrain_aPosition");
  quad_tree_.setupRenderData(program | "CDLODTerrain_uRenderData");

  uCamPos_ = engine::make_unique<gl::LazyUniform<glm::vec3>>(
      program, "CDLODTerrain_uCamPos");

  tex_unit_ = tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uHeightMap") = tex_unit;
  index_tex_unit_ = index_tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uHeightMapIndex") = index_tex_unit;

  gl::Uniform<int>(program, "CDLODTerrain_max_level") =
      tex_quad_tree_.max_node_level();

  // FIXME
  gl::Uniform<glm::ivec2>(program, "CDLODTerrain_uTexSize") =
      glm::ivec2(GlobalHeightMap::geom_w, GlobalHeightMap::geom_h);

  gl::Uniform<float>(program, "CDLODTerrain_uNodeDimension") =
      quad_tree_.node_dimension();
}

void TerrainMesh::render(Camera const& cam) {
  if (!uCamPos_) {
    throw std::logic_error("engine::cdlod::terrain requires a setup() call, "
                           "before the use of the render() function.");
  }

  tex_quad_tree_.update(cam);

  gl(ActiveTexture(GL_TEXTURE0 + tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, tex_quad_tree_.texture()));
  gl(ActiveTexture(GL_TEXTURE0 + index_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, tex_quad_tree_.index_texture()));

  uCamPos_->set(cam.transform()->pos());

  gl::FrontFace(gl::kCcw);
  gl::TemporaryEnable cullface{gl::kCullFace};

  quad_tree_.render(cam);

  gl(ActiveTexture(GL_TEXTURE0 + index_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
  gl(ActiveTexture(GL_TEXTURE0 + tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
}

}  // namespace cdlod
}  // namespace engine

#undef gl
