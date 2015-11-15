// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_

#include <memory>
#include "./streaming_info.h"
#include "../../collision/spherized_aabb.h"

namespace engine {
namespace cdlod {

class TexQuadTreeNode {
 public:
  TexQuadTreeNode(TexQuadTreeNode* parent,
                  double center_x, double center_z,
                  double size_x, double size_z,
                  int mip_level, unsigned index);

  bool collidesWithSphere(const Sphere& sphere) {
    return bbox_.collidesWithSphere(sphere);
  }

  void load();
  void load_files(Magick::Image& height,
                  Magick::Image& dx,
                  Magick::Image& dy) const;
  void load(Magick::Image& height,
            Magick::Image& dx,
            Magick::Image& dy);

  void age(StreamingInfo& streaming_info);
  void selectNodes(const glm::vec3& cam_pos,
                   const Frustum& frustum,
                   StreamingInfo& streaming_info,
                   std::set<TexQuadTreeNode*>& load_later,
                   bool force_load_now);
  void upload(StreamingInfo& streaming_info);

  double center_x() const { return x_; }
  double center_z() const { return z_; }
  double size_x() const { return sx_; }
  double size_z() const { return sz_; }

  int int_left_x() const { return int(x_) - int(sx_)/2; }
  double left_x() const {
    return level_ >= 0 ? int_left_x() : x_ - sx_ / 2.0;
  }
  int int_right_x() const { return int_left_x() + int(sx_); }
  double right_x() const { return left_x() + sx_; }

  int int_top_z() const { return int(z_) - int(sz_)/2; }
  double top_z() const {
    return level_ >= 0 ? int_top_z() : z_ - sz_ / 2.0;
  }
  int int_bottom_z() const { return int_top_z() + int(sz_); }
  double bottom_z() const { return top_z() + sz_; }

  int level() const { return level_; }
  int index() const { return index_; }

  std::string map_path(const char* base_path) const;
  std::string height_map_path() const;
  std::string dx_map_path() const;
  std::string dy_map_path() const;

  bool is_image_loaded() const { return !data_.empty(); }
  bool isUploadedToGPU() const { return isUploadedToGPU_; }
  int last_used() const { return last_used_; }
  const std::vector<TexelData>& data() const { return data_; }
  TexQuadTreeNode* getChild(int i) const {
    assert(0 <= i && i < 4); return children_[i].get();
  }

  static const int kTimeToLiveOnGPU = 1 << 8;

 private:
  using BBox = SpherizedAABBSat<GlobalHeightMap::tex_w, GlobalHeightMap::tex_h>;

  TexQuadTreeNode* parent_;
  double x_, z_, sx_, sz_;
  unsigned tex_w_, tex_h_, index_, data_start_offset_ = 0;
  int level_;
  BBox bbox_;
  std::unique_ptr<TexQuadTreeNode> children_[4];
  std::vector<TexelData> data_;
  bool isUploadedToGPU_ = false;

  int last_used_ = 0;
  // If a node is not used for this much time (frames), it will be unloaded.
  static const int kTimeToLiveInMemory = 1 << 16;
  static_assert(kTimeToLiveOnGPU < kTimeToLiveInMemory, "");

  template<typename T>
  void initChildInternal(int i);

  void initChild(int i);
};

}
}

#endif
