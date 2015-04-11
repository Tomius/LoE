// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <memory>
#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../global_height_map.h"
#include "../../camera.h"

namespace engine {
namespace cdlod {

class TexQuadTree {
  glm::ivec2 min_node_size_;
  GLubyte max_node_level_;
  TexQuadTreeNode root_;
  std::vector<GLubyte> texture_data_;
  gl::TextureBuffer tex_buffer_;
  gl::TextureBuffer tex_index_buffer_;

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
      node_count += 1 << (2*level); // pow(4, level)
    }
    size_t size = node_count*sizeof(TexQuadTreeNodeIndex);
    gl::Bind(tex_index_buffer_);
    tex_index_buffer_.data(size, nullptr, gl::kDynamicDraw);
    std::memset(gl::TextureBuffer::TypedMap<GLubyte>{}.data(), 0, size);
    gl::Unbind(tex_index_buffer_);
  }

 public:
  TexQuadTree(int w = GlobalHeightMap::w,
              int h = GlobalHeightMap::h,
              glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , max_node_level_(max_node_level(w, h))
      , root_{w/2, h/2, w, h, max_node_level_} {
    initTexIndexBuffer();
  }

  TexQuadTree(int w, int h, GLubyte max_depth)
      : min_node_size_{w >> max_depth, h >> max_depth}
      , max_node_level_(max_depth), root_{w/2, h/2, w, h, max_depth} {
    initTexIndexBuffer();
  }

  glm::ivec2 min_node_size() const {
    return min_node_size_;
  }

  TexQuadTreeNode const& root() const {
    return root_;
  }

  void update(Camera const& cam) {
    gl::Bind(tex_index_buffer_); {
      gl::TextureBuffer::TypedMap<TexQuadTreeNodeIndex> map;
      TexQuadTreeNodeIndex* indices = map.data();

      glm::vec3 cam_pos = cam.transform()->pos();
      texture_data_.clear();
      root_.selectNodes(cam_pos, cam.frustum(), 0, texture_data_, indices);
      root_.age();
    } // unmap indices

    gl::Bind(tex_buffer_);
    tex_buffer_.data(texture_data_, gl::kStreamDraw);
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
