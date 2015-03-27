// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_

#include <memory>
#include "../../collision/bounding_spherical_sector.h"

namespace engine {
namespace cdlod {

class TexQuadTreeNode {
 public:
  TexQuadTreeNode(int center_x, int center_z,
                  int size_x, int size_z, GLubyte mip_level);

  bool collidesWithSphere(const glm::vec3& center, float radius) {
    return bbox_.collidesWithSphere(center, radius);
  }

  void load();
  void initChildren();
  void selectNodes(const glm::vec3& cam_pos, const Frustum& frustum);

  int center_x() const { return x_; }
  int center_z() const { return z_; }
  int size_x() const { return sx_; }
  int size_z() const { return sz_; }
  int level() const { return level_; }

 private:
  GLubyte *data_ = nullptr;
  int x_, z_, sx_, sz_;
  GLubyte level_;
  BoundingSphericalSector bbox_;
  std::unique_ptr<TexQuadTreeNode> children_[4];
};

}
}

#endif
