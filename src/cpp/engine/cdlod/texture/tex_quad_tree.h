// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_H_

#include <mutex>
#include <memory>
#include <thread>
#include <algorithm>
#include <condition_variable>

#include "./tex_quad_tree_node.h"
#include "../../global_height_map.h"
#include "../../camera.h"
#include "../../oglwrap_all.h"

#define gl(func) OGLWRAP_CHECKED_FUNCTION(func)

namespace engine {
namespace cdlod {

class TexQuadTree {
 private:
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

  // prefetch
  size_t update_counter_ = 0;
  std::set<TexQuadTreeNode*> load_later_;
  std::mutex load_later_ownership_;
  std::condition_variable condition_variable_;
  bool worker_should_quit_ = false;
  std::thread worker_;
  int load_count_ = 0;
  bool worker_thread_should_sleep_ = true;

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
    normal_tex_buffer_.subData(offset, uploadSize, &normal_data_[last_data_size]);
    gl::Unbind(normal_tex_buffer_);
  }

  void imageLoaderThread() {
    while (!worker_should_quit_) {
      // wait until there's something to process
      std::unique_lock<std::mutex> lk(load_later_ownership_);
      condition_variable_.wait(lk, [this]{
        return worker_should_quit_ ||
          (!load_later_.empty() && !worker_thread_should_sleep_);
      });

      if (worker_should_quit_) { return; }

      // process one element of load later
      auto iter = load_later_.begin();
      (*iter)->load(&load_count_);
      load_later_.erase(iter);
    }
  }

 public:
  TexQuadTree(int w = GlobalHeightMap::tex_w,
              int h = GlobalHeightMap::tex_h,
              glm::ivec2 min_node_size = {256, 128})
      : min_node_size_{min_node_size}
      , max_node_level_(max_node_level(w, h))
      , root_{w/2, h/2, w, h, max_node_level_, 0}
      , worker_{[this]{imageLoaderThread();}} {
    initTexIndexBuffer();
    initTextures();
  }

  TexQuadTree(int w, int h, GLubyte max_depth)
      : min_node_size_{w >> max_depth, h >> max_depth}
      , max_node_level_(max_depth)
      , root_{w/2, h/2, w, h, max_node_level_, 0}
      , worker_{[this]{imageLoaderThread();}} {
    initTexIndexBuffer();
    initTextures();
  }

  ~TexQuadTree() {
    // signal the worker to quit
    worker_should_quit_ = true;
    condition_variable_.notify_one();

    // clean up
    gl(DeleteTextures(sizeof(textures_) / sizeof(textures_[0]), textures_));

    // wait until the worker has finished
    worker_.join();
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

  void update(Camera const& cam) {
    size_t last_data_size = height_data_.size();

    gl::Bind(index_tex_buffer_); {
      gl::TextureBuffer::TypedMap<TexQuadTreeNodeIndex> map;
      TexQuadTreeNodeIndex* indices = map.data();
      if (++update_counter_ % 1000 == 0) {
        std::memset(indices, 0, index_tex_buffer_size);
        last_data_size = 0;
        height_data_.clear();
        normal_data_.clear();
      }

      glm::vec3 cam_pos = cam.transform()->pos();

      { // lock load_later and select required notes
        worker_thread_should_sleep_ = true;
        std::unique_lock<std::mutex> lk(load_later_ownership_);
        load_count_ = 0;
        load_later_.clear(); // forget left over load_later data
        root_.selectNodes(cam_pos, cam.frustum(), height_data_,
                          normal_data_, indices, &load_count_, load_later_);

        if (load_count_ > 0) {
          std::cout << load_count_ << " image was loaded in frame "
                    << update_counter_ << std::endl;
        }
      }
      if (load_later_.size() > 0) {
        worker_thread_should_sleep_ = false;
        condition_variable_.notify_one(); // let the worker run
      }

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
