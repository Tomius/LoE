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

void TerrainMesh::setup(const gl::Program& program, int height_tex_unit,
                        int normal_tex_unit, int index_tex_unit) {
  gl::Use(program);

  quad_tree_.setupPositions(program | "CDLODTerrain_aPosition");
  quad_tree_.setupRenderData(program | "CDLODTerrain_aRenderData");

  uCamPos_ = engine::make_unique<gl::LazyUniform<glm::vec3>>(
      program, "CDLODTerrain_uCamPos");

  height_tex_unit_ = height_tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uHeightMap") = height_tex_unit;
  normal_tex_unit_ = normal_tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uNormalMap") = normal_tex_unit;
  index_tex_unit_ = index_tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uIndexTexture") = index_tex_unit;

  gl::Uniform<int>(program, "CDLODTerrain_max_level") =
      tex_quad_tree_.max_node_level();

  gl::Uniform<int>(program, "CDLODTerrain_max_height") =
      GlobalHeightMap::max_height;

  gl::Uniform<glm::ivec2>(program, "CDLODTerrain_uTexSize") =
      glm::ivec2(GlobalHeightMap::tex_w, GlobalHeightMap::tex_h);

  gl::Uniform<int>(program, "CDLODTerrain_uGeomDiv") = GlobalHeightMap::geom_div;

  gl::Uniform<float>(program, "CDLODTerrain_uNodeDimension") =
      GlobalHeightMap::node_dimension;

  gl::Uniform<float>(program, "CDLODTerrain_uLodLevelDistanceMultiplier") =
      GlobalHeightMap::lod_level_distance_multiplier;
}

void TerrainMesh::render(Camera const& cam) {
  if (!uCamPos_) {
    throw std::logic_error("engine::cdlod::terrain requires a setup() call, "
                           "before the use of the render() function.");
  }

  tex_quad_tree_.update(cam);

  gl(ActiveTexture(GL_TEXTURE0 + height_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, tex_quad_tree_.height_texture()));
  gl(ActiveTexture(GL_TEXTURE0 + normal_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, tex_quad_tree_.normal_texture()));
  gl(ActiveTexture(GL_TEXTURE0 + index_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, tex_quad_tree_.index_texture()));

  uCamPos_->set(cam.transform()->pos());

  gl::FrontFace(gl::kCcw);
  gl::TemporaryEnable cullface{gl::kCullFace};

  quad_tree_.render(cam);

  gl(ActiveTexture(GL_TEXTURE0 + index_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
  gl(ActiveTexture(GL_TEXTURE0 + normal_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
  gl(ActiveTexture(GL_TEXTURE0 + height_tex_unit_));
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
}

}  // namespace cdlod
}  // namespace engine

#undef gl
