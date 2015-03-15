// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_QUAD_TREE_NODE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "../height_map_interface.h"
#include "../collision/bounding_spherical_sector.h"

namespace engine {
namespace cdlod {

class QuadTreeNode {
 public:
  QuadTreeNode(GLshort x, GLshort z, GLubyte level, int dimension);

  GLushort size() { return dimension_ << level_; }

  bool collidesWithSphere(const glm::vec3& center, float radius) {
    return bbox_.collidesWithSphere(center, radius);
  }

  void initChildren();

  void selectNodes(const glm::vec3& cam_pos,
                   const Frustum& frustum,
                   QuadGridMesh& grid_mesh);

 private:
  GLshort x_, z_;
  GLushort dimension_;
  GLubyte level_;
  BoundingSphericalSector bbox_;
  std::unique_ptr<QuadTreeNode> tl_, tr_, bl_, br_;
  //static std::map<int, int> statistics;
};

}
}

#endif
