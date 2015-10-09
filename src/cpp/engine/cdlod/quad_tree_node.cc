// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./quad_tree_node.h"
#include "../misc.h"

namespace engine {
namespace cdlod {

QuadTreeNode::QuadTreeNode(int x, int z, GLubyte level)
    : x_(x), z_(z), level_(level)
    , bbox_{{x-size()/2, 0, z-size()/2},
            {x+size()/2, GlobalHeightMap::max_height, z+size()/2}} {}

// Returns false if the node doesn't have a single vertex inside the visible area
bool QuadTreeNode::isVisible(int x, int z, int level) {
  int s2 = GlobalHeightMap::node_dimension << (level - 1); // half size
  return (-s2 <= x && x <= GlobalHeightMap::geom_w + s2 &&
          -s2 <= z && z <= GlobalHeightMap::geom_h + s2);
}

void QuadTreeNode::initChildren() {
  assert(level_ > 0);
  children_inited_ = true;
  int s4 = size()/4;

  if (isVisible(x_-s4, z_+s4, level_-1)) {
    children_[0] = make_unique<QuadTreeNode>(x_-s4, z_+s4, level_-1);
  }
  if (isVisible(x_+s4, z_+s4, level_-1)) {
    children_[1] = make_unique<QuadTreeNode>(x_+s4, z_+s4, level_-1);
  }
  if (isVisible(x_-s4, z_-s4, level_-1)) {
    children_[2] = make_unique<QuadTreeNode>(x_-s4, z_-s4, level_-1);
  }
  if (isVisible(x_+s4, z_-s4, level_-1)) {
    children_[3] = make_unique<QuadTreeNode>(x_+s4, z_-s4, level_-1);
  }
}

void QuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                               const Frustum& frustum,
                               QuadGridMesh& grid_mesh) {
  float scale = 1 << level_;
  float lod_range = GlobalHeightMap::lod_level_distance_multiplier
                    * scale * GlobalHeightMap::node_dimension;

  if (!bbox_.collidesWithFrustum(frustum)) { return; }

  // If we can cover the whole area or if we are a leaf
  Sphere sphere{cam_pos, lod_range};
  if (!bbox_.collidesWithSphere(sphere) || level_ == 0) {
    grid_mesh.addToRenderList(x_, z_, scale, level_);
  } else {
    if (!children_inited_) {
      initChildren();
    }
    bool cc[4]{}; // children collision

    for (int i = 0; i < 4; ++i) {
      if (children_[i]) {
        cc[i] = children_[i]->collidesWithSphere(sphere);
        if (cc[i]) {
          // Ask child to render what we can't
          children_[i]->selectNodes(cam_pos, frustum, grid_mesh);
        }
      }
    }

    // Render what the children didn't do
    grid_mesh.addToRenderList(x_, z_, scale, level_,
                              !cc[0], !cc[1], !cc[2], !cc[3]);
  }
}

}  // namespace cdlod
}  // namespace engine
