// Copyright (c) 2014, Tamas Csala

#include <thread>
#include <algorithm>
#include "./quad_tree.h"
#include "../misc.h"

namespace engine {
namespace cdlod {

QuadTree::Node::Node(GLshort x, GLshort z, GLubyte level, int dimension)
    : x(x), z(z), dimension(dimension), level(level)
    , bbox{{x-size()/2, 0, z-size()/2}, {x+size()/2, 100, z+size()/2}}
    , tl(nullptr), tr(nullptr), bl(nullptr), br(nullptr) {}

void QuadTree::Node::initChildren() {
  assert(level > 0);
  tl = std::unique_ptr<Node>(new Node(x-size()/4, z+size()/4, level-1, dimension));
  tr = std::unique_ptr<Node>(new Node(x+size()/4, z+size()/4, level-1, dimension));
  bl = std::unique_ptr<Node>(new Node(x-size()/4, z-size()/4, level-1, dimension));
  br = std::unique_ptr<Node>(new Node(x+size()/4, z-size()/4, level-1, dimension));
}

void QuadTree::Node::selectNodes(const glm::vec3& cam_pos,
                                 const Frustum& frustum,
                                 QuadGridMesh& grid_mesh) {
  float scale = 1 << level;
  float lod_range = pow(2.5, level) * dimension;

  if (!bbox.collidesWithFrustum(frustum)) { return; }

  // if we can cover the whole area or if we are a leaf
  if (!bbox.collidesWithSphere(cam_pos, lod_range) || level == 0) {
    grid_mesh.addToRenderList(x, z, scale, level);
  } else {
    if (!tl) {
      initChildren();
    }
    bool btl = tl->collidesWithSphere(cam_pos, lod_range);
    bool btr = tr->collidesWithSphere(cam_pos, lod_range);
    bool bbl = bl->collidesWithSphere(cam_pos, lod_range);
    bool bbr = br->collidesWithSphere(cam_pos, lod_range);

    // Ask childs to render what we can't
    if (btl) {
      tl->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (btr) {
      tr->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (bbl) {
      bl->selectNodes(cam_pos, frustum, grid_mesh);
    }
    if (bbr) {
      br->selectNodes(cam_pos, frustum, grid_mesh);
    }

    // Render, what the childs didn't do
    grid_mesh.addToRenderList(x, z, scale, level, !btl, !btr, !bbl, !bbr);
  }
}

}  // namespace cdlod
}  // namespace engine
