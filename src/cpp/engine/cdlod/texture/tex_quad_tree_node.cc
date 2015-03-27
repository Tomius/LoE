// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../misc.h"

namespace engine {
namespace cdlod {

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
  data_ = new GLubyte[w*h];
  image.write(0, 0, w, h, "R", MagickCore::CharPixel, data_);
}

void TexQuadTreeNode::initChildren() {
  assert(level_ > 0);

  // Chilren node sizes (tl = top left, br = bottom right)
  int tl_sx = sx_/2,       tl_sz = sz_/2;
  int tr_sx = sx_ - sx_/2, tr_sz = sz_/2;
  int bl_sx = sx_/2,       bl_sz = sz_ - sz_/2;
  int br_sx = sx_ - sx_/2, br_sz = sz_ - sz_/2;

  // top left
  children_[0] = make_unique<TexQuadTreeNode>(
    x_ - (tl_sx - tl_sx/2), z_ - (tl_sz - tl_sz/2), tl_sx, tl_sz, level_-1);
  // top right
  children_[1] = make_unique<TexQuadTreeNode>(
    x_ + tr_sx/2, z_ - (tr_sz - tr_sz/2), tr_sx, tr_sz, level_-1);
  // bottom left
  children_[2] = make_unique<TexQuadTreeNode>(
    x_ - (bl_sx - bl_sx/2), z_ + bl_sz/2, bl_sx, bl_sz, level_-1);
  // bottom right
  children_[3] = make_unique<TexQuadTreeNode>(
    x_ + br_sx/2, z_ + br_sz/2, br_sx, br_sz, level_-1);
}

void TexQuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                                  const Frustum& frustum) {
  float lod_range = 1.01 * std::max(sx_, sz_);

  if (!bbox_.collidesWithFrustum(frustum)) { return; }

  // if we can cover the whole area or if we are a leaf
  if (!bbox_.collidesWithSphere(cam_pos, lod_range) || level_ == 0) {
    load(); // load in the texture
  } else {
    // generate children nodes if we can't cover the whole area
    if (!children_[0]) {
      initChildren();
    }

    bool children_cover_whole_area = false;
    for (int i = 0; i < 4; ++i) {
      if (children_[i]->collidesWithSphere(cam_pos, lod_range)) {
        children_[i]->selectNodes(cam_pos, frustum);
        children_cover_whole_area = true;
      }
    }

    // If we have to render something, we have to load the texture too.
    if (!children_cover_whole_area) {
      load();
    }
  }
}

}  // namespace cdlod
}  // namespace engine
