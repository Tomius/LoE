// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_QUAD_TREE_NODE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "../global_height_map.h"
#include "../collision/bounding_spherical_sector.h"

namespace engine {
namespace cdlod {

class QuadTreeNode {
 public:
  QuadTreeNode(int x, int z, GLubyte level, int dimension);

  int size() { return dimension_ << level_; }

  bool collidesWithSphere(const glm::vec3& center, float radius) {
    return bbox_.collidesWithSphere(center, radius);
  }

  static bool isOutsideUsefulArea(int x, int z, int level, int dimension);

  void initChildren();

  void selectNodes(const glm::vec3& cam_pos,
                   const Frustum& frustum,
                   QuadGridMesh& grid_mesh);

 private:
  using BBox = BoundingSphericalSector<GlobalHeightMap::geom_w, GlobalHeightMap::geom_h>;

  int x_, z_;
  GLushort dimension_;
  GLubyte level_;
  BBox bbox_;
  std::unique_ptr<QuadTreeNode> children_[4];
  bool children_inited_ = false;
};

}
}

#endif
