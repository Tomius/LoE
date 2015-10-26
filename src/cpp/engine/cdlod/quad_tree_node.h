// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_QUAD_TREE_NODE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "../global_height_map.h"
#include "../collision/spherized_aabb.h"

namespace engine {
namespace cdlod {

class QuadTreeNode {
 public:
  QuadTreeNode(double x, double z, int level);

  double scale() const { return pow(2, level_); }
  double size() { return GlobalHeightMap::node_dimension * scale(); }

  bool collidesWithSphere(const Sphere& sphere) const {
    return bbox_.collidesWithSphere(sphere);
  }

  void selectNodes(const glm::vec3& cam_pos,
                   const Frustum& frustum,
                   QuadGridMesh& grid_mesh);

 private:
  using BBox = SpherizedAABBSat<GlobalHeightMap::tex_w, GlobalHeightMap::tex_h>;

  double x_, z_;
  int level_;
  BBox bbox_;
  std::unique_ptr<QuadTreeNode> children_[4];
  bool children_inited_ = false;

  static bool isVisible(double x, double z, int level);

  void initChildren();
};

}
}

#endif
