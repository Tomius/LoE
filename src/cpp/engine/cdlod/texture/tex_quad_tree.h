// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <memory>
#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../camera.h"

namespace engine {
namespace cdlod {

class TexQuadTree {
  glm::ivec2 min_node_size_;

  TexQuadTreeNode root_;

  GLubyte max_node_level(int w, int h) const {
    int x_depth = 0;
    while (w >> x_depth > min_node_size_.x) {
      x_depth++;
    }
    int y_depth = 0;
    while (h >> y_depth > min_node_size_.y) {
      y_depth++;
    }

    return std::max(std::max(x_depth, y_depth), 0);
  }

 public:
  TexQuadTree(int w, int h, glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , root_{w/2, h/2, w, h, max_node_level(w, h)} {}

  TexQuadTree(int w, int h, GLubyte max_depth)
      : min_node_size_{w >> max_depth, h >> max_depth}
      , root_{w/2, h/2, w, h, max_depth} {}

  glm::ivec2 min_node_size() const {
    return min_node_size_;
  }

  TexQuadTreeNode const& root() const {
    return root_;
  }

  void render(const engine::Camera& cam) {
    glm::vec3 cam_pos = cam.transform()->pos();
    root_.selectNodes(cam_pos, cam.frustum());
    root_.age();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
