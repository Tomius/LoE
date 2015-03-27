// Copyright (c) 2015, Tamas Csala

#ifndef LOD_TEX_TERRAIN_H_
#define LOD_TEX_TERRAIN_H_

#include "engine/cdlod/texture/tex_quad_tree.h"
#include "engine/global_terrain_setting.h"

class TexTerrain : public engine::GameObject {
 public:
  explicit TexTerrain(engine::GameObject* parent)
    : GameObject(parent), quad_tree_(global_terrain::w, global_terrain::h) {}

 private:
  engine::cdlod::TexQuadTree quad_tree_;

  virtual void render() override {
    quad_tree_.render(*scene_->camera());
  }
};

#endif  // LOD_TEX_TERRAIN_H_
