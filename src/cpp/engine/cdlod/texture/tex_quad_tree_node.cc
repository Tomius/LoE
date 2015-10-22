// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../misc.h"

int frame_counter = 0;

namespace engine {
namespace cdlod {

TexQuadTreeNode::TexQuadTreeNode(int x, int z, int sx, int sz,
                                 GLubyte level, unsigned index)
    : x_(x), z_(z), sx_(sx), sz_(sz), index_(index), level_(level)
    , bbox_{{x-sx/2, 0, z-sz/2},
            {x+(sx-sx/2), GlobalHeightMap::max_height, z+(sz-sz/2)}} {}

static int last_frame_counter = 0;
static int load_counter = 0;

void TexQuadTreeNode::load() {
  load_counter++;

  int tx = x_ - sx_/2, ty = z_ - sz_/2;

  { // height tex
    char height_tex[200];
    sprintf(height_tex, "%s/%d/%d/%d.png",
            GlobalHeightMap::height_texture_base_path,
            level_, tx, ty);

    Magick::Image height_image(height_tex);
    tex_w_ = height_image.columns();
    tex_h_ = height_image.rows();
    height_data_.resize(tex_w_*tex_h_);

    height_image.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, height_data_.data());
  }


  // placeholder for interleaving
  std::vector<GLushort> temp;
  temp.resize(tex_w_*tex_h_);
  normal_data_.resize(tex_w_*tex_h_);

  { // dx tex
    char dx_tex[200];
    sprintf(dx_tex, "%s/%d/%d/%d.png",
            GlobalHeightMap::dx_texture_base_path,
            level_, tx, ty);

    Magick::Image dx_image(dx_tex);
    assert(tex_w_ == dx_image.columns());
    assert(tex_h_ == dx_image.rows());

    dx_image.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      normal_data_[i].dx = temp[i];
    }
  }

  { //dy tex
    char dy_tex[200];
    sprintf(dy_tex, "%s/%d/%d/%d.png",
            GlobalHeightMap::dy_texture_base_path,
            level_, tx, ty);

    Magick::Image dy_image(dy_tex);
    assert(tex_w_ == dy_image.columns());
    assert(tex_h_ == dy_image.rows());

    dy_image.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      normal_data_[i].dy = temp[i];
    }
  }
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
          x_ - (sx - sx/2), z_ - (sz - sz/2), sx, sz, level_-1, 4*index_+i+1);
    } break;
    case 1: { // top right
      int sx = sx_ - sx_/2, sz = sz_/2;
      children_[1] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ - (sz - sz/2), sx, sz, level_-1, 4*index_+i+1);
    } break;
    case 2: { // bottom left
      int sx = sx_/2, sz = sz_ - sz_/2;
      children_[2] = make_unique<TexQuadTreeNode>(
          x_ - (sx - sx/2), z_ + sz/2, sx, sz, level_-1, 4*index_+i+1);
    } break;
    case 3: { // bottom right
      int sx = sx_ - sx_/2, sz = sz_ - sz_/2;
      children_[3] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ + sz/2, sx, sz, level_-1, 4*index_+i+1);
    } break;
    default: {
      throw new std::out_of_range("Tried to index "
          + std::to_string(i) + "th child of a quadtree node.");
    }
  }
}

void TexQuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                                  const Frustum& frustum,
                                  std::vector<GLushort>& height_data,
                                  std::vector<DerivativeInfo>& normal_data,
                                  TexQuadTreeNodeIndex* indices) {
  // if (frame_counter > last_frame_counter) {
  //   if (load_counter != 0) {
  //     std::cout << load_counter << " image was loaded in frame " << last_frame_counter << std::endl;
  //     load_counter = 0;
  //   }
  //   last_frame_counter = frame_counter;
  // }

  float lod_range = 2*sqrt(double(sx_)*sx_ + double(sz_)*sz_) * (1 << GlobalHeightMap::geom_div);

  // check if the node is visible
  if (!bbox_.collidesWithFrustum(frustum)) {
    return;
  }

  Sphere sphere{cam_pos, lod_range};
  if (bbox_.collidesWithSphere(sphere))  {
    upload(height_data, normal_data, indices);

    if (level_ != 0) {
      for (int i = 0; i < 4; ++i) {
        auto& child = children_[i];

        if (!child) {
          initChild(i);
        }

        // call selectNodes on the child (recursive)
        if (child->collidesWithSphere(sphere)) {
          child->selectNodes(cam_pos, frustum, height_data, normal_data, indices);
        }
      }
    }
  }

  last_used_ = 0;
}

void TexQuadTreeNode::upload(std::vector<GLushort>& height_data,
                             std::vector<DerivativeInfo>& normal_data,
                             TexQuadTreeNodeIndex* indices) {
  if (height_data_.empty()) {
    load();
  }

  if (indices[index_].tex_size_x == 0) {
    assert (height_data.size() == normal_data.size());
    GLint offset = height_data.size();

    indices[index_] = TexQuadTreeNodeIndex{
      GLushort(offset >> 16), GLushort(offset % (1 << 16)),
      GLushort(tex_w_), GLushort(tex_h_)
    };
    height_data.insert(height_data.end(), height_data_.begin(), height_data_.end());
    normal_data.insert(normal_data.end(), normal_data_.begin(), normal_data_.end());
  }
}

}  // namespace cdlod
}  // namespace engine
