// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <memory>
#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../global_height_map.h"
#include "../../camera.h"
#include "../../oglwrap_all.h"

#define gl(func) OGLWRAP_CHECKED_FUNCTION(func)

namespace engine {
namespace cdlod {

class TexQuadTree {
  glm::ivec2 min_node_size_;
  GLubyte max_node_level_;
  TexQuadTreeNode root_;

  size_t last_data_alloc_ = 1024*1024;
  size_t index_tex_buffer_size;
  std::vector<GLushort> height_data_;
  std::vector<DerivativeInfo> normal_data_;
  gl::TextureBuffer height_tex_buffer_;
  gl::TextureBuffer normal_tex_buffer_;
  gl::TextureBuffer index_tex_buffer_;
  GLuint textures_[3];
  size_t update_counter = 0;

  GLubyte max_node_level(int w, int h) const {
    int x_depth = 1;
    while (w >> x_depth > min_node_size_.x) {
      x_depth++;
    }
    int y_depth = 1;
    while (h >> y_depth > min_node_size_.y) {
      y_depth++;
    }

    // The x_depth and y_depth go one past the desired value
    return std::max(x_depth, y_depth) - 1;
  }

  void initTexIndexBuffer() {
    size_t node_count = 0;
    for (int level = 0; level <= max_node_level_; ++level) {
      node_count += 1 << (2*level); // == pow(4, level)
    }
    index_tex_buffer_size = node_count*sizeof(TexQuadTreeNodeIndex);
    gl::Bind(index_tex_buffer_);
    index_tex_buffer_.data(index_tex_buffer_size, nullptr, gl::kDynamicDraw);
    std::memset(gl::TextureBuffer::Map{}.data(), 0, index_tex_buffer_size);
    gl::Unbind(index_tex_buffer_);
  }

  void initTextures () {
    gl(GenTextures(sizeof(textures_) / sizeof(textures_[0]), textures_));

    gl(BindTexture(GL_TEXTURE_BUFFER, index_texture()));
    gl(TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA16UI, index_tex_buffer_.expose()));

    gl::Bind(height_tex_buffer_);
    height_tex_buffer_.data(last_data_alloc_ * sizeof(GLushort), nullptr, gl::kDynamicDraw);

    gl(BindTexture(GL_TEXTURE_BUFFER, height_texture()));
    gl(TexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, height_tex_buffer_.expose()));

    gl::Bind(normal_tex_buffer_);
    normal_tex_buffer_.data(last_data_alloc_ * sizeof(DerivativeInfo), nullptr, gl::kDynamicDraw);

    gl(BindTexture(GL_TEXTURE_BUFFER, normal_texture()));
    gl(TexBuffer(GL_TEXTURE_BUFFER, GL_RG16UI, normal_tex_buffer_.expose()));

    gl::Unbind(normal_tex_buffer_);
    gl(BindTexture(GL_TEXTURE_BUFFER, 0));
  }

 public:
  TexQuadTree(int w = GlobalHeightMap::tex_w,
              int h = GlobalHeightMap::tex_h,
              glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , max_node_level_(max_node_level(w, h))
      , root_{w/2, h/2, w, h, max_node_level_} {
    initTexIndexBuffer();
    initTextures();
  }

  TexQuadTree(int w, int h, GLubyte max_depth)
      : min_node_size_{w >> max_depth, h >> max_depth}
      , max_node_level_(max_depth)
      , root_{w/2, h/2, w, h, max_node_level_} {
    initTexIndexBuffer();
    initTextures();
  }

  ~TexQuadTree() {
    gl(DeleteTextures(sizeof(textures_) / sizeof(textures_[0]), textures_));
  }

  glm::ivec2 min_node_size() const {
    return min_node_size_;
  }

  TexQuadTreeNode const& root() const {
    return root_;
  }

  int max_node_level() const {
    return max_node_level_;
  }

  GLuint height_texture() const {
    return textures_[0];
  }

  GLuint normal_texture() const {
    return textures_[1];
  }

  GLuint index_texture() const {
    return textures_[2];
  }

  void enlargeBuffers(size_t new_data_size) {
    last_data_alloc_ = 2*new_data_size;

    gl::Bind(height_tex_buffer_);
    height_tex_buffer_.data(last_data_alloc_ * sizeof(GLushort), nullptr, gl::kDynamicDraw);
    height_tex_buffer_.subData(0, height_data_);

    gl::Bind(normal_tex_buffer_);
    normal_tex_buffer_.data(last_data_alloc_ * sizeof(DerivativeInfo), nullptr, gl::kDynamicDraw);
    normal_tex_buffer_.subData(0, normal_data_);
  }

  void uploadNewData(size_t last_data_size) {
    gl::Bind(height_tex_buffer_);
    size_t offset = last_data_size * sizeof(GLushort);
    size_t uploadSize = (height_data_.size()-last_data_size) * sizeof(GLushort);
    assert(offset+uploadSize <= last_data_alloc_ * sizeof(GLushort));
    height_tex_buffer_.subData(offset, uploadSize, &height_data_[last_data_size]);

    gl::Bind(normal_tex_buffer_);
    offset = last_data_size * sizeof(DerivativeInfo);
    uploadSize = (normal_data_.size()-last_data_size) * sizeof(DerivativeInfo);
    assert(offset+uploadSize <= last_data_alloc_ * sizeof(DerivativeInfo));
    height_tex_buffer_.subData(offset, uploadSize, &normal_data_[last_data_size]);
    gl::Unbind(normal_tex_buffer_);
  }

  void update(Camera const& cam) {
    size_t last_data_size = height_data_.size();

    gl::Bind(index_tex_buffer_); {
      gl::TextureBuffer::TypedMap<TexQuadTreeNodeIndex> map;
      TexQuadTreeNodeIndex* indices = map.data();
      if (++update_counter % 1000 == 0) {
        std::memset(indices, 0, index_tex_buffer_size);
        last_data_size = 0;
        height_data_.clear();
        normal_data_.clear();
      }

      glm::vec3 cam_pos = cam.transform()->pos();
      root_.selectNodes(cam_pos, cam.frustum(), 0,
                        height_data_, normal_data_, indices);
      root_.age();
    } // unmap indices

    size_t new_data_size = height_data_.size();
    if (new_data_size > last_data_alloc_) {
      enlargeBuffers (new_data_size);
    } else {
      uploadNewData (last_data_size);
    }
  }
};

}  // namespace cdlod
}  // namespace engine

#undef gl

#endif
