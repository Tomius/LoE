// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_

#include <memory>
#include "../../collision/spherized_aabb.h"

namespace engine {
namespace cdlod {

struct TexQuadTreeNodeIndex {
  GLushort data_offset_hi, data_offset_lo;
  GLushort tex_size_x, tex_size_y;
};

class TexQuadTreeNode {
 public:
  TexQuadTreeNode(int center_x, int center_z,
                  int size_x, int size_z, GLubyte mip_level);

  bool collidesWithSphere(const Sphere& sphere) {
    return bbox_.collidesWithSphere(sphere);
  }

  void load();
  void age();
  void initChild(int i);
  void selectNodes(const glm::vec3& cam_pos, const Frustum& frustum, int index,
                   std::vector<GLubyte>& texture_data,
                   TexQuadTreeNodeIndex* indices);
  void upload(int index, std::vector<GLubyte>& texture_data,
              TexQuadTreeNodeIndex* indices);

  int center_x() const { return x_; }
  int center_z() const { return z_; }
  int size_x() const { return sx_; }
  int size_z() const { return sz_; }
  int level() const { return level_; }

 private:
  using BBox = SpherizedAABBSat<GlobalHeightMap::tex_w, GlobalHeightMap::tex_h>;

  int x_, z_, sx_, sz_, tex_w_, tex_h_;
  int last_used_ = 0;
  GLubyte level_;
  BBox bbox_;
  std::unique_ptr<TexQuadTreeNode> children_[4];
  std::vector<GLubyte> data_;

  // If a node is not used for this much time (frames), it will be unloaded.
  static const int time_to_live_ = 256;
};

}
}

#endif
