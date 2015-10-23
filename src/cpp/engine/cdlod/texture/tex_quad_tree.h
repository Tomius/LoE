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

namespace engine {
namespace cdlod {

class TexQuadTree {
 public:
  TexQuadTree(int w = GlobalHeightMap::tex_w,
              int h = GlobalHeightMap::tex_h,
              glm::ivec2 min_node_size = {256, 128});
  TexQuadTree(int w, int h, GLubyte max_depth);
  ~TexQuadTree();

  glm::ivec2 min_node_size() const { return min_node_size_; }
  TexQuadTreeNode const& root() const { return root_; }
  int max_node_level() const { return max_node_level_; }
  GLuint height_texture() const { return textures_[0]; }
  GLuint normal_texture() const { return textures_[1]; }
  GLuint index_texture() const { return textures_[2]; }

  void update(Camera const& cam);

 private:
  glm::ivec2 min_node_size_;
  GLubyte max_node_level_;
  TexQuadTreeNode root_;

  size_t last_data_alloc_ = 1024*1024;
  std::vector<GLushort> height_data_;
  std::vector<DerivativeInfo> normal_data_;
  std::vector<TexQuadTreeNodeIndex> index_data_;
  gl::TextureBuffer height_tex_buffer_;
  gl::TextureBuffer normal_tex_buffer_;
  gl::TextureBuffer index_tex_buffer_;
  GLuint textures_[3];

  // anync load data
  size_t update_counter_ = 0;
  std::set<TexQuadTreeNode*> load_later_;
  std::mutex load_later_ownership_;
  std::condition_variable condition_variable_;
  bool worker_should_quit_ = false;
  std::thread worker_;
  bool worker_thread_should_sleep_ = true;

  GLubyte max_node_level(int w, int h) const;

  void initTexIndexBuffer();
  void initTextures ();

  void enlargeBuffers(size_t new_data_size);
  void uploadNewData(size_t last_data_size);

  void imageLoaderThread();
};

}  // namespace cdlod
}  // namespace engine

#endif
