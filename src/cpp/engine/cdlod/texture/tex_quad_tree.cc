// Copyright (c) 2015, Tamas Csala

#include "./tex_quad_tree.h"

#define gl(func) OGLWRAP_CHECKED_FUNCTION(func)

namespace engine {
namespace cdlod {

GLubyte TexQuadTree::max_node_level(int w, int h) const {
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

void TexQuadTree::initTexIndexBuffer() {
  size_t node_count = 0;
  for (int level = 0; level <= max_node_level_; ++level) {
    node_count += 1 << (2*level); // == pow(4, level)
  }
  streaming_info_.index_data.resize(node_count); // default ctor - all zeros

  gl::Bind(streaming_info_.index_tex_buffer);
  streaming_info_.index_tex_buffer.data(
      streaming_info_.index_data, gl::kDynamicDraw);
  gl::Unbind(streaming_info_.index_tex_buffer);
}

void TexQuadTree::initTextures () {
  gl(GenTextures(sizeof(textures_) / sizeof(textures_[0]), textures_));

  gl(BindTexture(GL_TEXTURE_BUFFER, index_texture()));
  gl(TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA16UI,
      streaming_info_.index_tex_buffer.expose()));

  gl::Bind(streaming_info_.height_tex_buffer);
  streaming_info_.height_tex_buffer.data(
      streaming_info_.last_data_alloc * sizeof(GLushort),
      nullptr, gl::kDynamicDraw);

  gl(BindTexture(GL_TEXTURE_BUFFER, height_texture()));
  gl(TexBuffer(GL_TEXTURE_BUFFER, GL_R16UI,
      streaming_info_.height_tex_buffer.expose()));

  gl::Bind(streaming_info_.normal_tex_buffer);
  streaming_info_.normal_tex_buffer.data(
      streaming_info_.last_data_alloc * sizeof(DerivativeInfo),
      nullptr, gl::kDynamicDraw);

  gl(BindTexture(GL_TEXTURE_BUFFER, normal_texture()));
  gl(TexBuffer(GL_TEXTURE_BUFFER, GL_RG16UI,
     streaming_info_.normal_tex_buffer.expose()));

  gl::Unbind(streaming_info_.normal_tex_buffer);
  gl(BindTexture(GL_TEXTURE_BUFFER, 0));
}

void TexQuadTree::imageLoaderThread() {
  while (!worker_should_quit_) {
    TexQuadTreeNode* nodeToProcess = nullptr;

    {
      // wait until there's something to process
      std::unique_lock<std::mutex> lock{load_later_ownership_};
      condition_variable_.wait(lock, [this]{
        return worker_should_quit_ ||
          (load_count_ < 5 && !load_later_.empty() && !worker_thread_should_sleep_);
      });

      if (worker_should_quit_) { return; }

      // remove one element from the load_later_ set.
      auto iter = load_later_.begin();
      nodeToProcess = *iter;
      load_later_.erase(iter);
    }

    if (nodeToProcess->is_image_loaded()) {
      continue;
    }

    // load the image without modifying the node
    // this is slow - this should not block the rendering.
    Magick::Image height, dx, dy;
    nodeToProcess->load_files(height, dx, dy);

    { // update the node data using loaded image files (blocking but fast)
      std::unique_lock<std::mutex> lock{load_later_ownership_};
      nodeToProcess->load(height, dx, dy);
      load_count_++;
    }
  }
}

TexQuadTree::TexQuadTree(int w /*= GlobalHeightMap::tex_w*/,
                         int h /*= GlobalHeightMap::tex_h*/,
                         glm::ivec2 min_node_size /*= {256, 128}*/)
    : min_node_size_{min_node_size}
    , max_node_level_(max_node_level(w, h))
    , root_(w/2, h/2, w, h, max_node_level_, 0)
    , worker_{[this]{imageLoaderThread();}} {
  initTexIndexBuffer();
  initTextures();
}

TexQuadTree::TexQuadTree(int w, int h, GLubyte max_depth)
    : min_node_size_{w >> max_depth, h >> max_depth}
    , max_node_level_(max_depth)
    , root_(w/2, h/2, w, h, max_node_level_, 0)
    , worker_{[this]{imageLoaderThread();}} {
  initTexIndexBuffer();
  initTextures();
}

TexQuadTree::~TexQuadTree() {
  // signal the worker to quit
  worker_should_quit_ = true;
  condition_variable_.notify_one();

  // clean up
  gl(DeleteTextures(sizeof(textures_) / sizeof(textures_[0]), textures_));

  // wait until the worker has finished
  worker_.join();
}

void TexQuadTree::update(Camera const& cam) {
  // temporarily stop worker thread, and upload
  // data for the current rendering frame
  {
    worker_thread_should_sleep_ = true;
    std::unique_lock<std::mutex> lock{load_later_ownership_};

    glm::vec3 cam_pos = cam.transform()->pos();
    bool force_syncronous_load = (streaming_info_.uploaded_texel_count == 0);

    // Forget load_later data that we couldn't load since last frame. If some
    // texture wasn't loaded by the worker thread, but it is still needed, then
    // the selectNodes will add it again. If it won't add it - then it's not
    // required anymore to render, so it's a good thing that we dropped it.
    load_later_.clear();
    load_count_ = 0;
    root_.selectNodes(cam_pos, cam.frustum(), streaming_info_,
                      load_later_, force_syncronous_load);

    // unload unused textures, and keep track of last use times
    root_.age();
  } // lock expires here

  // start worker thread if there's work to do
  if (load_later_.size() > 0) {
    worker_thread_should_sleep_ = false;
    condition_variable_.notify_one(); // let the worker run
  }
}

}  // namespace cdlod
}  // namespace engine

#undef gl
