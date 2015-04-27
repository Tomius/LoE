// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./quad_tree_node.h"
#include "../misc.h"

namespace engine {
namespace cdlod {

QuadTreeNode::QuadTreeNode(int x, int z, GLubyte level, int dimension)
    : x_(x), z_(z), dimension_(dimension), level_(level)
    , bbox_{{x-size()/2, 0, z-size()/2}, {x+size()/2, 100, z+size()/2}} {}

bool QuadTreeNode::isOutsideUsefulArea(int x, int z, int level, int dimension) {
  int size = dimension << level;
  return !(x+size < 0 || x < GlobalHeightMap::geom_w ||
           z+size < 0 || z < GlobalHeightMap::geom_h);
}

void QuadTreeNode::initChildren() {
  assert(level_ > 0);
  children_inited_ = true;
  if (!isOutsideUsefulArea(x_-size()/4, z_+size()/4, level_-1, dimension_)) {
    children_[0] = make_unique<QuadTreeNode>(
        x_-size()/4, z_+size()/4, level_-1, dimension_);
  }
  if (!isOutsideUsefulArea(x_+size()/4, z_+size()/4, level_-1, dimension_)) {
    children_[1] = make_unique<QuadTreeNode>(
        x_+size()/4, z_+size()/4, level_-1, dimension_);
  }
  if (!isOutsideUsefulArea(x_-size()/4, z_-size()/4, level_-1, dimension_)) {
    children_[2] = make_unique<QuadTreeNode>(
        x_-size()/4, z_-size()/4, level_-1, dimension_);
  }
  if (!isOutsideUsefulArea(x_+size()/4, z_-size()/4, level_-1, dimension_)) {
    children_[3] = make_unique<QuadTreeNode>(
        x_+size()/4, z_-size()/4, level_-1, dimension_);
  }
}

void QuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                               const Frustum& frustum,
                               QuadGridMesh& grid_mesh) {
  float scale = 1 << level_;
  float lod_range = scale * dimension_;

  if (!bbox_.collidesWithFrustum(frustum)) { return; }

  // if we can cover the whole area or if we are a leaf
  if (!bbox_.collidesWithSphere(cam_pos, lod_range) || level_ == 0) {
    grid_mesh.addToRenderList(x_, z_, scale, level_);
  } else {
    if (!children_inited_) {
      initChildren();
    }
    bool cc[4]{}; // children collision

    for (int i = 0; i < 4; ++i) {
      if (children_[i]) {
        cc[i] = children_[i]->collidesWithSphere(cam_pos, lod_range);
        if (cc[i]) {
          // Ask child to render what we can't
          children_[i]->selectNodes(cam_pos, frustum, grid_mesh);
        }
      }
    }

    // Render, what the childs didn't do
    grid_mesh.addToRenderList(
        x_, z_, scale, level_, !cc[0], !cc[1], !cc[2], !cc[3]);
  }
}

}  // namespace cdlod
}  // namespace engine
