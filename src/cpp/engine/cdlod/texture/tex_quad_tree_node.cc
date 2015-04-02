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
    , bbox_{{x-sx/2, 0, z-sz/2}, {x+(sx-sx/2), 100, z+(sz-sz/2)}} {}

// TODO
// it just decodes a jpg files to examine performance
void TexQuadTreeNode::load() {
  if (data_) {
    return;
  }

  std::string path = "src/resources/gmted2010/4/000_000.jpg";
  Magick::Image image(path);
  size_t w = image.columns();
  size_t h = image.rows();
  data_ = std::unique_ptr<GLubyte>{new GLubyte[w*h]};
  image.write(0, 0, w, h, "R", MagickCore::CharPixel, data_.get());
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
                                  const Frustum& frustum) {
  float lod_range = 1.01 * std::max(sx_, sz_);

  // check if the node is visible
  if (!bbox_.collidesWithFrustum(frustum)) {
    return;
  }

  // if we can cover the whole area or if we are a leaf
  if (!bbox_.collidesWithSphere(cam_pos, lod_range) || level_ == 0) {
    load(); // load in the texture
  } else {
    bool children_cover_whole_area = true;
    for (int i = 0; i < 4; ++i) {
      auto& child = children_[i];

      if (!child) {
        initChild(i);
      }

      // call selectNodes on the child (recursive)
      if (child->collidesWithSphere(cam_pos, lod_range)) {
        child->selectNodes(cam_pos, frustum);
      } else {
        children_cover_whole_area = false;
      }
    }

    // If we have to render something, we have to load the texture too.
    if (!children_cover_whole_area) {
      load();
    }
  }

  last_used_ = 0;
}

}  // namespace cdlod
}  // namespace engine
