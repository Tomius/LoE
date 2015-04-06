// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_H_
#define ENGINE_CDLOD_QUAD_TREE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "./quad_tree_node.h"
#include "../camera.h"
#include "../height_map_interface.h"

namespace engine {
namespace cdlod {

class QuadTree {
  QuadGridMesh mesh_;
  int node_dimension_;

  QuadTreeNode root_;

  GLubyte max_node_level(int w, int h) const {
    int x_depth = 1;
    while (w >> x_depth > node_dimension_) {
      x_depth++;
    }
    int y_depth = 1;
    while (h >> y_depth > node_dimension_) {
      y_depth++;
    }

    // The x_depth and y_depth go one past the desired value
    return std::max(x_depth, y_depth) - 1;
  }

 public:
  QuadTree(const HeightMapInterface& hmap, int node_dimension = 32)
      : mesh_(node_dimension), node_dimension_(node_dimension)
      , root_(hmap.w()/2, hmap.h()/2,
              max_node_level(hmap.w(), hmap.h()), node_dimension) {}

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
    mesh_.render();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
