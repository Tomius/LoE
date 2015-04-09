// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <memory>
#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../global_height_map.h"
#include "../../camera.h"

namespace engine {
namespace cdlod {

class TexQuadTree : public engine::GameObject {
  glm::ivec2 min_node_size_;
  TexQuadTreeNode root_;

  GLubyte max_node_level(int w, int h) const {
    int x_depth = 1;
    while (w >> x_depth > min_node_size_.x) {
      x_depth++;
    }
    int y_depth = 1;
    while (h >> y_depth > min_node_size_.y) {
      y_depth++;
    }

    // The x_depth and y_depth go one past the desired value
    return std::max(x_depth, y_depth) - 1;
  }

 public:
  TexQuadTree(GameObject* parent,
              int w = GlobalHeightMap::w,
              int h = GlobalHeightMap::h,
              glm::ivec2 min_node_size = {256, 128})
      : GameObject(parent), min_node_size_{min_node_size}
      , root_{w/2, h/2, w, h, max_node_level(w, h)} {}

  TexQuadTree(GameObject* parent, int w, int h, GLubyte max_depth)
      : GameObject(parent), min_node_size_{w >> max_depth, h >> max_depth}
      , root_{w/2, h/2, w, h, max_depth} {}

  glm::ivec2 min_node_size() const {
    return min_node_size_;
  }

  TexQuadTreeNode const& root() const {
    return root_;
  }

  virtual void render() override {
    auto cam = scene_->camera();
    glm::vec3 cam_pos = cam->transform()->pos();
    root_.selectNodes(cam_pos, cam->frustum());
    root_.age();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
