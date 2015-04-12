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
  std::vector<GLubyte> texture_data_;
  gl::TextureBuffer tex_buffer_;
  gl::TextureBuffer index_tex_buffer_;
  GLuint textures_[2];

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
    gl::Bind(index_tex_buffer_);
    index_tex_buffer_.data(size, nullptr, gl::kDynamicDraw);
    std::memset(gl::TextureBuffer::Map{}.data(), 0, size);
    gl::Unbind(index_tex_buffer_);
  }

 public:
  TexQuadTree(int w = GlobalHeightMap::w,
              int h = GlobalHeightMap::h,
              glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , max_node_level_(max_node_level(w, h))
      , root_{w/2, h/2, w, h, max_node_level_} {
    initTexIndexBuffer();
    gl(GenTextures(2, textures_));
  }

  TexQuadTree(int w, int h, GLubyte max_depth)
      : min_node_size_{w >> max_depth, h >> max_depth}
      , max_node_level_(max_depth), root_{w/2, h/2, w, h, max_depth} {
    initTexIndexBuffer();
    gl(GenTextures(2, textures_));
  }

  ~TexQuadTree() {
    gl(DeleteTextures(2, textures_));
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

  GLuint texture() const {
    return textures_[0];
  }

  GLuint index_texture() const {
    return textures_[1];
  }

  void update(Camera const& cam) {
    gl::Bind(index_tex_buffer_); {
      gl::TextureBuffer::TypedMap<TexQuadTreeNodeIndex> map;
      TexQuadTreeNodeIndex* indices = map.data();

      texture_data_.clear();
      glm::vec3 cam_pos = cam.transform()->pos();
      root_.selectNodes(cam_pos, cam.frustum(), 0, texture_data_, indices);
      root_.age();
    } // unmap indices
    // for (int i = 0; i < 335*168; ++i) {
    //   if (texture_data_[i] != 0) {
    //     std::cout << std::dec << i << ":" << std::hex << (int)texture_data_[i] << " ";
    //   }
    // }
    // std::cout << std::endl;
    // std::terminate();

    gl(BindTexture(GL_TEXTURE_BUFFER, textures_[1]));
    gl(TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA16UI, index_tex_buffer_.expose()));

    gl::Bind(tex_buffer_);
    tex_buffer_.data(texture_data_, gl::kStreamDraw);
    gl::Unbind(tex_buffer_);

    gl(BindTexture(GL_TEXTURE_BUFFER, textures_[0]));
    gl(TexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, tex_buffer_.expose()));

    gl(BindTexture(GL_TEXTURE_BUFFER, 0));
  }
};

}  // namespace cdlod
}  // namespace engine

#undef gl

#endif
