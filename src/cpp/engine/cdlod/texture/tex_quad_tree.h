// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <memory>
#include <algorithm>
#include "./tex_quad_tree_node.h"

namespace engine {
namespace cdlod {

class TexQuadTree {
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
  TexQuadTree(int w = kTerrainWidth, int h = kTerrainHeight, glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , root_{w/2, h/2, w, h, max_node_level(w, h)} {
    // what a beautiful line of code
    root_.setSiblings({{&root_, &root_, &root_, &root_, &root_, &root_, &root_, &root_}});
    root_.generateImage();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
