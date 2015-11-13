// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../misc.h"

#include <thread>
#include <chrono>
using namespace std::literals::chrono_literals;

#define gl(func) OGLWRAP_CHECKED_FUNCTION(func)

namespace engine {
namespace cdlod {

TexQuadTreeNode::TexQuadTreeNode(TexQuadTreeNode* parent,
                                 double x, double z, double sx, double sz,
                                 GLubyte level, unsigned index)
    : parent_(parent)
    , x_(x), z_(z)
    , sx_(sx), sz_(sz)
    , index_(index), level_(level)
    , bbox_{{left_x(), 0, top_z()},
            {right_x(), GlobalHeightMap::max_height, bottom_z()}} {}

void TexQuadTreeNode::load() {
  Magick::Image height, dx, dy;
  load_files(height, dx, dy);
  load(height, dx, dy);
}

std::string TexQuadTreeNode::map_path(const char* base_path) const {
  char file_path[200];
  if (level_ >= 0) {
    int tx = int_left_x(), ty = int_top_z();
    sprintf(file_path, "%s/%d/%d/%d.png", base_path, level_, tx, ty);
  } else {
    double tx = left_x(), ty = top_z();
    sprintf(file_path, "%s/%d/%f/%f.png", base_path, level_, tx, ty);
  }

  return file_path;
}

std::string TexQuadTreeNode::height_map_path() const {
  return map_path(GlobalHeightMap::height_texture_base_path);
}

std::string TexQuadTreeNode::dx_map_path() const {
  return map_path(GlobalHeightMap::dx_texture_base_path);
}

std::string TexQuadTreeNode::dy_map_path() const {
  return map_path(GlobalHeightMap::dy_texture_base_path);
}

void TexQuadTreeNode::load_files(Magick::Image& height,
                                 Magick::Image& dx,
                                 Magick::Image& dy) const {
  if (is_image_loaded()) {
    return;
  }

  if (level_ >= 0) {
    height.read(height_map_path());
    dx.read(dx_map_path());
    dy.read(dy_map_path());
  } else {
    assert(parent_ != nullptr);
    std::string src_height = parent_->height_map_path();
    std::string dst_height = height_map_path();
    std::string command =
    "convert " + src_height + " -filter 'Hermite' -resize 682 foo_hermite.png";
    abort();
  }
}

void TexQuadTreeNode::load(Magick::Image& height,
                           Magick::Image& dx,
                           Magick::Image& dy) {
  if (is_image_loaded()) {
    return;
  }

  // placeholder for interleaving
  std::vector<GLushort> temp;

  { // height tex
    tex_w_ = height.columns();
    tex_h_ = height.rows();
    data_.resize(tex_w_*tex_h_);
    temp.resize(tex_w_*tex_h_);

    height.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      data_[i].height = temp[i];
    }
  }


  { // dx tex
    assert(tex_w_ == dx.columns());
    assert(tex_h_ == dx.rows());

    dx.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      data_[i].dx = temp[i];
    }
  }

  { //dy tex
    assert(tex_w_ == dy.columns());
    assert(tex_h_ == dy.rows());

    dy.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      data_[i].dy = temp[i];
    }
  }
}

void TexQuadTreeNode::age() {
  last_used_++;

  for (auto& child : children_) {
    if (child) {
      // unload child if its age would exceed the ttl
      if (child->last_used_ >= kTimeToLiveInMemory) {
        child.reset();
      } else {
        child->age();
      }
    }
  }
}

void TexQuadTreeNode::initChild(int i) {
  assert(level_ > 0);

  int left_sx = int(sx_)/2;
  int right_sx = int(sx_) - int(sx_)/2;
  int top_sz = int(sz_)/2;
  int bottom_sz = int(sz_) - int(sz_)/2;

  int left_cx = int(x_) - (left_sx - left_sx/2);
  int right_cx = int(x_) + right_sx/2;
  int top_cz = int(z_) - (top_sz - top_sz/2);
  int bottom_cz = int(z_) + bottom_sz/2;

  switch (i) {
    case 0: { // top left
      children_[0] = make_unique<TexQuadTreeNode>(
          this, left_cx, top_cz, left_sx, top_sz, level_-1, i);
    } break;
    case 1: { // top right
      children_[1] = make_unique<TexQuadTreeNode>(
          this, right_cx, top_cz, right_sx, top_sz, level_-1, i);
    } break;
    case 2: { // bottom left
      children_[2] = make_unique<TexQuadTreeNode>(
          this, left_cx, bottom_cz, left_sx, bottom_sz, level_-1, i);
    } break;
    case 3: { // bottom right
      children_[3] = make_unique<TexQuadTreeNode>(
          this, right_cx, bottom_cz, right_sx, bottom_sz, level_-1, i);
    } break;
    default: {
      throw std::out_of_range("Tried to index "
          + std::to_string(i) + "th child of a quadtree node.");
    }
  }
}

void TexQuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                                  const Frustum& frustum,
                                  StreamingInfo& streaming_info,
                                  std::set<TexQuadTreeNode*>& load_later,
                                  bool force_load_now) {
  float lod_range = sx_;

  Sphere sphere{cam_pos, lod_range};
  if (bbox_.collidesWithFrustum(frustum) && bbox_.collidesWithSphere(sphere))  {
    last_used_ = 0;
    if (force_load_now) {
      load();
      upload(streaming_info);
    } else {
      if (is_image_loaded()) {
        upload(streaming_info);
      } else {
        load_later.insert(this);
      }
    }


    if (level_ != 0) {
      for (int i = 0; i < 4; ++i) {
        auto& child = children_[i];

        if (!child) {
          initChild(i);
        }

        // call selectNodes on the child (recursive)
        child->selectNodes(cam_pos, frustum, streaming_info,
                           load_later, force_load_now);
      }
    }
  }
}


static void enlargeBuffers(StreamingInfo& streaming_info) {
  size_t new_data_alloc = 2*streaming_info.last_data_alloc;
  GLint max_texture_buffer_size;
  gl(GetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_texture_buffer_size));
  assert(new_data_alloc <= max_texture_buffer_size);

  gl::Bind(streaming_info.copyBuffer);
  size_t copy_buffer_size = streaming_info.last_data_alloc * sizeof(GLushort);
  streaming_info.copyBuffer.data(copy_buffer_size, nullptr, gl::kStreamCopy);

  // save data to copy buffer, resize, and copy data back
  gl::Bind(streaming_info.tex_buffer);
  gl(CopyBufferSubData(GL_TEXTURE_BUFFER, // src
                       GL_COPY_WRITE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       copy_buffer_size // size
  ));
  streaming_info.tex_buffer.data(
    new_data_alloc * sizeof(GLushort), nullptr, gl::kDynamicDraw);
  gl(CopyBufferSubData(GL_COPY_WRITE_BUFFER, // src
                       GL_TEXTURE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       copy_buffer_size // size
  ));

  streaming_info.last_data_alloc = new_data_alloc;
}

static void SubData(gl::TextureBuffer& buffer,
                    size_t offset, size_t length, void* src_data) {
  gl::Bind(buffer);
  buffer.subData(offset, length, src_data);
}

void TexQuadTreeNode::upload(StreamingInfo& streaming_info) {
  if (!is_image_loaded()) {
    load();
  }

  if (isUploadedToGPU_) {
    return;
  }

  bool found_empty_place = false;
  // look for empty places first
  for (int i = streaming_info.empty_places.size()-1; i >= 0; --i) {
    TexQuadTreeNode* data_owner = streaming_info.empty_places[i];
    // the texture sizes vary, we need an usused place with enough memory
    if (data_.size() == data_owner->data_.size()) {
      data_start_offset_ = data_owner->data_start_offset_;
      data_owner->isUploadedToGPU_ = false;
      found_empty_place = true;
      streaming_info.empty_places.erase(streaming_info.empty_places.begin()+i);
      break;
    }
  }

  if (!found_empty_place) {
    size_t new_texel_count = streaming_info.uploaded_texel_count
      + (sizeof(TextureInfo) + data_.size()*sizeof(TexelData)) / sizeof(GLushort);

    if (new_texel_count > streaming_info.last_data_alloc) {
      enlargeBuffers(streaming_info);
    }

    data_start_offset_ = streaming_info.uploaded_texel_count;
    streaming_info.uploaded_texel_count = new_texel_count;
  }

  if (parent_) {
    unsigned offset_of_start_offset = parent_->data_start_offset_ + 2 * (index_+1);
    GLushort data_start_offset_hi_and_lo[2] = {
      GLushort(data_start_offset_ >> 16),
      GLushort(data_start_offset_ % (1 << 16))};
    SubData(
      streaming_info.tex_buffer,
      offset_of_start_offset * sizeof(GLushort),
      sizeof(data_start_offset_hi_and_lo),
      &data_start_offset_hi_and_lo);
  }

  TextureInfo texinfo = {GLushort(tex_w_), GLushort(tex_h_)};
  SubData(
    streaming_info.tex_buffer,
    data_start_offset_ * sizeof(GLushort),
    sizeof(TextureInfo),
    &texinfo);

  SubData(
    streaming_info.tex_buffer,
    data_start_offset_ * sizeof(GLushort) + sizeof(TextureInfo),
    data_.size() * sizeof(TexelData),
    data_.data());

  isUploadedToGPU_ = true;
}

}  // namespace cdlod
}  // namespace engine
