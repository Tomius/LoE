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

  mesh_.setupPositions(program | "CDLODTerrain_aPosition");
  mesh_.setupRenderData(program | "CDLODTerrain_uRenderData");

  uCamPos_ = engine::make_unique<gl::LazyUniform<glm::vec3>>(
      program, "CDLODTerrain_uCamPos");

  tex_unit_ = tex_unit;
  gl::UniformSampler(program, "CDLODTerrain_uHeightMap") = tex_unit;
  gl::Uniform<glm::vec2>(program, "CDLODTerrain_uTexSize") =
      glm::vec2(GlobalHeightMap::w, GlobalHeightMap::h);
  gl::Uniform<float>(program, "CDLODTerrain_uNodeDimension") =
      mesh_.node_dimension();


  gl::BindToTexUnit(height_map_tex_, tex_unit);
  GlobalHeightMap::upload(height_map_tex_);
  height_map_tex_.minFilter(gl::kLinearMipmapLinear);
  height_map_tex_.magFilter(gl::kNearest);
  height_map_tex_.wrapS(gl::kClampToEdge);
  height_map_tex_.wrapT(gl::kClampToEdge);
  height_map_tex_.wrapP(gl::kClampToEdge);
  gl::Unbind(height_map_tex_);
}

void TerrainMesh::render(const Camera& cam) {
  if (!uCamPos_) {
    throw std::logic_error("engine::cdlod::terrain requires a setup() call, "
                           "before the use of the render() function.");
  }

  gl::BindToTexUnit(height_map_tex_, tex_unit_);

  uCamPos_->set(cam.transform()->pos());

  gl::FrontFace(gl::kCcw);
  gl::TemporaryEnable cullface{gl::kCullFace};

  mesh_.render(cam);

  gl::UnbindFromTexUnit(height_map_tex_, tex_unit_);
}

}  // namespace cdlod
}  // namespace engine
