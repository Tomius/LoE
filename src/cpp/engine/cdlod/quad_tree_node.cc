// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./quad_tree_node.h"
#include "../misc.h"

namespace engine {
namespace cdlod {


QuadTreeNode::QuadTreeNode(GLshort x, GLshort z, GLubyte level, int dimension)
    : x_(x), z_(z), dimension_(dimension), level_(level)
    , bbox_{{x-size()/2, 0, z-size()/2}, {x+size()/2, 100, z+size()/2}}
    , tl_(nullptr), tr_(nullptr), bl_(nullptr), br_(nullptr) {}

void QuadTreeNode::initChildren() {
  assert(level_ > 0);
  tl_ = make_unique<QuadTreeNode>(x_-size()/4, z_+size()/4, level_-1, dimension_);
  tr_ = make_unique<QuadTreeNode>(x_+size()/4, z_+size()/4, level_-1, dimension_);
  bl_ = make_unique<QuadTreeNode>(x_-size()/4, z_-size()/4, level_-1, dimension_);
  br_ = make_unique<QuadTreeNode>(x_+size()/4, z_-size()/4, level_-1, dimension_);
}

void QuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                               const Frustum& frustum,
                               QuadGridMesh& grid_mesh) {
  float scale = 1 << level_;
  float lod_range = pow(2.5, level_) * dimension_;

  if (!bbox_.collidesWithFrustum(frustum)) { return; }

  // if we can cover the whole area or if we are a leaf
  if (!bbox_.collidesWithSphere(cam_pos, lod_range) || level_ == 0) {
    grid_mesh.addToRenderList(x_, z_, scale, level_);
  } else {
    if (!tl_) {
      initChildren();
    }
    bool btl = tl_->collidesWithSphere(cam_pos, lod_range);
    bool btr = tr_->collidesWithSphere(cam_pos, lod_range);
    bool bbl = bl_->collidesWithSphere(cam_pos, lod_range);
    bool bbr = br_->collidesWithSphere(cam_pos, lod_range);

    // Ask childs to render what we can't
    if (btl) {
      tl_->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (btr) {
      tr_->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (bbl) {
      bl_->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (bbr) {
      br_->selectNodes(cam_pos, frustum, grid_mesh);
    }

    // Render, what the childs didn't do
    grid_mesh.addToRenderList(x_, z_, scale, level_, !btl, !btr, !bbl, !bbr);
  }
}

}  // namespace cdlod
}  // namespace engine
