// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_H_
#define ENGINE_CDLOD_QUAD_TREE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "./quad_tree_node.h"
#include "../camera.h"
#include "../global_height_map.h"

namespace engine {
namespace cdlod {

class QuadTree {
  QuadGridMesh mesh_;
  int node_dimension_;
  size_t w_, h_;
  QuadTreeNode root_; // must be initialized after w_, h_, and node_dim_

  GLubyte max_node_level() const {
    int x_depth = 0;
    while ((node_dimension_ << x_depth) < w_) {
      x_depth++;
    }
    int y_depth = 0;
    while ((node_dimension_ << y_depth) < h_) {
      y_depth++;
    }

    return std::max(x_depth, y_depth);
  }

 public:
  QuadTree(int node_dimension = 64)
      : mesh_(node_dimension), node_dimension_(node_dimension)
      , w_(GlobalHeightMap::geom_w)
      , h_(GlobalHeightMap::geom_h)
      , root_(w_/2, h_/2, max_node_level(), node_dimension) {}

  int node_dimension() const {
    return node_dimension_;
  }

  void setupPositions(gl::VertexAttrib attrib) {
    mesh_.setupPositions(attrib);
  }

  void setupRenderData(gl::VertexAttrib attrib) {
    mesh_.setupRenderData(attrib);
  }

  void render(const engine::Camera& cam) {
    mesh_.clearRenderList();
    glm::vec3 cam_pos = cam.transform()->pos();
    root_.selectNodes(cam_pos, cam.frustum(), mesh_);
    /*std::cout << "Node count: " << mesh_.node_count() << std::endl;
    for (std::pair<int, int> pair : mesh_.statistics()) {
      std::cout << pair.first << ": " << pair.second << std::endl;
    }*/
    mesh_.render();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
