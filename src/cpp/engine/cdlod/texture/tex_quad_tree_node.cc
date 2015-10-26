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

TexQuadTreeNode::TexQuadTreeNode(int x, int z, int sx, int sz,
                                 GLubyte level, unsigned index)
    : x_(x), z_(z), sx_(sx), sz_(sz), index_(index), level_(level)
    , bbox_{{x-sx/2, 0, z-sz/2},
            {x+(sx-sx/2), GlobalHeightMap::max_height, z+(sz-sz/2)}} {}

void TexQuadTreeNode::load() {
  Magick::Image height, dx, dy;
  load_files(height, dx, dy);
  load(height, dx, dy);
}

void TexQuadTreeNode::load_files(Magick::Image& height,
                                 Magick::Image& dx,
                                 Magick::Image& dy) const {
  if (is_image_loaded()) {
    return;
  }

  // std::this_thread::sleep_for(5ms);

  char file_path[200];
  int tx = x_ - sx_/2, ty = z_ - sz_/2;

  sprintf(file_path, "%s/%d/%d/%d.png",
          GlobalHeightMap::height_texture_base_path, level_, tx, ty);
  height.read(file_path);

  sprintf(file_path, "%s/%d/%d/%d.png",
          GlobalHeightMap::dx_texture_base_path, level_, tx, ty);
  dx.read(file_path);

  sprintf(file_path, "%s/%d/%d/%d.png",
          GlobalHeightMap::dy_texture_base_path, level_, tx, ty);
  dy.read(file_path);
}

void TexQuadTreeNode::load(Magick::Image& height,
                           Magick::Image& dx,
                           Magick::Image& dy) {
  if (is_image_loaded()) {
    return;
  }

  { // height tex
    tex_w_ = height.columns();
    tex_h_ = height.rows();
    height_data_.resize(tex_w_*tex_h_);
    height.write(0, 0, tex_w_, tex_h_, "R",
                 MagickCore::ShortPixel, height_data_.data());
  }


  // placeholder for interleaving
  std::vector<GLushort> temp;
  temp.resize(tex_w_*tex_h_);
  normal_data_.resize(tex_w_*tex_h_);

  { // dx tex
    assert(tex_w_ == dx.columns());
    assert(tex_h_ == dx.rows());
    dx.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
    for (size_t i = 0; i < temp.size(); ++i) {
      normal_data_[i].dx = temp[i];
    }
  }

  { //dy tex
    assert(tex_w_ == dy.columns());
    assert(tex_h_ == dy.rows());

    dy.write(0, 0, tex_w_, tex_h_, "R", MagickCore::ShortPixel, temp.data());
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
                                  StreamingInfo& streaming_info,
                                  std::set<TexQuadTreeNode*>& load_later,
                                  bool force_load_now) {
  float lod_range = sqrt(double(sx_)*sx_ + double(sz_)*sz_) *
                    (1 << (GlobalHeightMap::geom_div+1));

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


static void enlargeBuffers(size_t& last_data_alloc,
                           size_t new_texel_count,
                           gl::TextureBuffer& height_tex_buffer,
                           gl::TextureBuffer& normal_tex_buffer) {
  size_t new_data_alloc = 2*new_texel_count;

  static gl::BufferObject<gl::BufferType::kCopyWriteBuffer> copyBuffer;
  gl::Bind(copyBuffer);
  gl(BindBuffer(GL_COPY_READ_BUFFER, copyBuffer.expose()));

  // save data to copy buffer, resize, and copy data back
  gl::Bind(height_tex_buffer);
  gl(CopyBufferSubData(GL_TEXTURE_BUFFER, // src
                       GL_COPY_WRITE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       last_data_alloc * sizeof(GLushort) // size
  ));
  height_tex_buffer.data(new_data_alloc * sizeof(GLushort), nullptr,
                         gl::kDynamicDraw);
  gl(CopyBufferSubData(GL_COPY_READ_BUFFER, // src
                       GL_TEXTURE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       last_data_alloc * sizeof(GLushort) // size
  ));

  gl::Bind(normal_tex_buffer);
  gl(CopyBufferSubData(GL_TEXTURE_BUFFER, // src
                       GL_COPY_WRITE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       last_data_alloc * sizeof(DerivativeInfo) // size
  ));
  normal_tex_buffer.data(new_data_alloc * sizeof(DerivativeInfo), nullptr,
                         gl::kDynamicDraw);
  gl(CopyBufferSubData(GL_COPY_READ_BUFFER, // src
                       GL_TEXTURE_BUFFER, // dst
                       0, // read offset
                       0, // write offset
                       last_data_alloc * sizeof(DerivativeInfo) // size
  ));

  last_data_alloc = new_data_alloc;
}

static void EfficientSubData(gl::TextureBuffer& buffer,
                             size_t offset, size_t length, void* src_data) {
  gl::Bind(buffer);
  void* dst_data = gl(MapBufferRange(GL_TEXTURE_BUFFER, offset, length,
                      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
                      GL_MAP_UNSYNCHRONIZED_BIT));
  std::memcpy(dst_data, src_data, length);
  assert(glUnmapBuffer(GL_TEXTURE_BUFFER) == GL_TRUE);
}

void TexQuadTreeNode::upload(StreamingInfo& streaming_info) {
  if (!is_image_loaded()) {
    load();
  }

  if (streaming_info.index_data[index_].tex_size_x == 0) {
    // look for empty places first
    for (int i = 0; i < streaming_info.data_owners.size(); ++i) {
      TexQuadTreeNode* data_owner = streaming_info.data_owners[i];

      // we can use allocated memory that wasn't used for a while
      if (data_owner->last_used() > data_owner->kTimeToLiveOnGPU) {
        // the texture sizes vary, we need an usused place with enough memory
        if (height_data_.size() <= data_owner->height_data_.size()) {
          GLint offset =
            (streaming_info.index_data[data_owner->index_].data_offset_hi << 16) +
            streaming_info.index_data[data_owner->index_].data_offset_lo;

          streaming_info.index_data[index_].data_offset_hi = offset >> 16;
          streaming_info.index_data[index_].data_offset_lo = offset % (1 << 16);
          streaming_info.index_data[index_].tex_size_x     = tex_w_;
          streaming_info.index_data[index_].tex_size_y     = tex_h_;
          EfficientSubData(
              streaming_info.index_tex_buffer,
              index_ * sizeof(TexQuadTreeNodeIndex),
              sizeof(TexQuadTreeNodeIndex),
              &streaming_info.index_data[index_]);

          streaming_info.index_data[data_owner->index_] = TexQuadTreeNodeIndex{};
          EfficientSubData(
              streaming_info.index_tex_buffer,
              data_owner->index_ * sizeof(TexQuadTreeNodeIndex),
              sizeof(TexQuadTreeNodeIndex),
              &streaming_info.index_data[data_owner->index_]);

          EfficientSubData(
            streaming_info.height_tex_buffer,
            offset * sizeof(GLushort),
            height_data_.size()*sizeof(GLushort),
            height_data_.data()
          );

          EfficientSubData(
            streaming_info.normal_tex_buffer,
            offset * sizeof(DerivativeInfo),
            normal_data_.size()*sizeof(DerivativeInfo),
            normal_data_.data()
          );

          streaming_info.data_owners[i] = this;

          return;
        }
      }
    }

    // if we did not find an empty place
    size_t new_texel_count =
      streaming_info.uploaded_texel_count + height_data_.size();
    if (new_texel_count > streaming_info.last_data_alloc) {
      enlargeBuffers(
          streaming_info.last_data_alloc,
          new_texel_count,
          streaming_info.height_tex_buffer,
          streaming_info.normal_tex_buffer
      );
    }

    GLint offset = streaming_info.uploaded_texel_count;

    streaming_info.index_data[index_].data_offset_hi = offset >> 16;
    streaming_info.index_data[index_].data_offset_lo = offset % (1 << 16);
    streaming_info.index_data[index_].tex_size_x     = tex_w_;
    streaming_info.index_data[index_].tex_size_y     = tex_h_;

    EfficientSubData(
        streaming_info.index_tex_buffer,
        index_ * sizeof(TexQuadTreeNodeIndex),
        sizeof(TexQuadTreeNodeIndex),
        &streaming_info.index_data[index_]);

    EfficientSubData(
        streaming_info.height_tex_buffer,
        offset * sizeof(GLushort),
        height_data_.size()*sizeof(GLushort),
        height_data_.data()
    );

    EfficientSubData(
        streaming_info.normal_tex_buffer,
        offset * sizeof(DerivativeInfo),
        normal_data_.size()*sizeof(DerivativeInfo),
        normal_data_.data()
    );

    streaming_info.data_owners.push_back(this);
    streaming_info.uploaded_texel_count = new_texel_count;
  }
}

}  // namespace cdlod
}  // namespace engine
