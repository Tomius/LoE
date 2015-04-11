// Copyright (c) 2015, Tamas Csala

#include "./terrain_mesh.h"
#include "../oglwrap_all.h"

namespace engine {
namespace cdlod {

TerrainMesh::TerrainMesh(engine::ShaderManager* manager) {
  gl::ShaderSource vs_src{"engine/cdlod_terrain.vert"};
  manager->publish("engine/cdlod_terrain.vert", vs_src);
}

void TerrainMesh::setup(const gl::Program& program, int tex_unit) {
  gl::Use(program);

  quad_tree_.setupPositions(program | "CDLODTerrain_aPosition");
  quad_tree_.setupRenderData(program | "CDLODTerrain_uRenderData");

  uCamPos_ = engine::make_unique<gl::LazyUniform<glm::vec3>>(
      program, "CDLODTerrain_uCamPos");

  tex_unit_ = tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uHeightMap") = tex_unit;
  gl::Uniform<glm::ivec2>(program, "CDLODTerrain_uTexSize") =
      glm::ivec2(GlobalHeightMap::w, GlobalHeightMap::h);
  gl::Uniform<float>(program, "CDLODTerrain_uNodeDimension") =
      quad_tree_.node_dimension();
}

void TerrainMesh::render(Camera const& cam) {
  if (!uCamPos_) {
    throw std::logic_error("engine::cdlod::terrain requires a setup() call, "
                           "before the use of the render() function.");
  }

  tex_quad_tree_.update(cam);

  gl::BindToTexUnit(height_map_tex_, tex_unit_);

  uCamPos_->set(cam.transform()->pos());

  gl::FrontFace(gl::kCcw);
  gl::TemporaryEnable cullface{gl::kCullFace};

  quad_tree_.render(cam);

  gl::UnbindFromTexUnit(height_map_tex_, tex_unit_);
}

}  // namespace cdlod
}  // namespace engine
