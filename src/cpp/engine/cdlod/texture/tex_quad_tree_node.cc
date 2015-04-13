// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../misc.h"

namespace engine {
namespace cdlod {

// If a node is not used for this much time (frames), it will be unloaded.
int TexQuadTreeNode::time_to_live_ = 256;

TexQuadTreeNode::TexQuadTreeNode(int x, int z, int sx, int sz, GLubyte level)
    : x_(x), z_(z), sx_(sx), sz_(sz), level_(level)
    , bbox_{{x-sx/2, 0, z-sz/2}, {x+(sx-sx/2), 200, z+(sz-sz/2)}} {}

void TexQuadTreeNode::load() {
  char str[100];
  int tx = x_ - sx_/2, ty = z_ - sz_/2;
  sprintf(str, "src/resources/gmted2010_75/%d/%d/%d.jpg", level_, tx, ty);

  Magick::Image image(str);
  tex_w_ = image.columns();
  tex_h_ = image.rows();
  data_.resize(tex_w_*tex_h_);
  image.write(0, 0, tex_w_, tex_h_, "R", MagickCore::CharPixel, data_.data());
}

void TexQuadTreeNode::age() {
  last_used_++;

  for (auto& child : children_) {
    if (child) {
      // unload child if its age would exceed the ttl
      if (child->last_used_ >= time_to_live_) {
        child.reset();
      } else {
        child->age();
      }
    }
  }
}

void TexQuadTreeNode::initChild(int i) {
  assert(level_ > 0);

  switch (i) {
    case 0: { // top left
      int sx = sx_/2, sz = sz_/2;
      children_[0] = make_unique<TexQuadTreeNode>(
          x_ - (sx - sx/2), z_ - (sz - sz/2), sx, sz, level_-1);
    } break;
    case 1: { // top right
      int sx = sx_ - sx_/2, sz = sz_/2;
      children_[1] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ - (sz - sz/2), sx, sz, level_-1);
    } break;
    case 2: { // bottom left
      int sx = sx_/2, sz = sz_ - sz_/2;
      children_[2] = make_unique<TexQuadTreeNode>(
          x_ - (sx - sx/2), z_ + sz/2, sx, sz, level_-1);
    } break;
    case 3: { // bottom right
      int sx = sx_ - sx_/2, sz = sz_ - sz_/2;
      children_[3] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ + sz/2, sx, sz, level_-1);
    } break;
    default: {
      throw new std::out_of_range("Tried to index "
          + std::to_string(i) + "th child of a quadtree node.");
    }
  }
}

void TexQuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                                  const Frustum& frustum,
                                  int index,
                                  std::vector<GLubyte>& texture_data,
                                  TexQuadTreeNodeIndex* indices) {
  float lod_range = 1.01 * sqrt(double(sx_)*sx_ + double(sz_)*sz_);

  // check if the node is visible
  if (!bbox_.collidesWithFrustum(frustum)) {
    return;
  }

  upload(index, texture_data, indices);

  if (bbox_.collidesWithSphere(cam_pos, lod_range) && level_ != 0)  {
    for (int i = 0; i < 4; ++i) {
      auto& child = children_[i];

      if (!child) {
        initChild(i);
      }

      // call selectNodes on the child (recursive)
      if (child->collidesWithSphere(cam_pos, lod_range)) {
        child->selectNodes(cam_pos, frustum, 4*index+i+1, texture_data, indices);
      }
    }
  }

  last_used_ = 0;
}

void TexQuadTreeNode::upload(int index, std::vector<GLubyte>& texture_data,
                             TexQuadTreeNodeIndex* indices) {
  if (data_.empty()) {
    load();
  }

  GLint offset = sizeof(GLubyte) * texture_data.size();
  indices[index] = TexQuadTreeNodeIndex{
    GLushort(offset >> 16), GLushort(offset % (1 << 16)),
    GLushort(tex_w_), GLushort(tex_h_)
  };
  texture_data.insert(texture_data.end(), data_.begin(), data_.end());
}

}  // namespace cdlod
}  // namespace engine
