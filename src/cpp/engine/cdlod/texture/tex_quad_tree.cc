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
  index_data_.resize(node_count); // default ctor - all zeros

  gl::Bind(index_tex_buffer_);
  index_tex_buffer_.data(index_data_, gl::kDynamicDraw);
  gl::Unbind(index_tex_buffer_);
}

void TexQuadTree::initTextures () {
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

void TexQuadTree::enlargeBuffers(size_t new_data_size) {
  last_data_alloc_ = 2*new_data_size;

  gl::Bind(height_tex_buffer_);
  height_tex_buffer_.data(last_data_alloc_ * sizeof(GLushort), nullptr, gl::kDynamicDraw);
  height_tex_buffer_.subData(0, height_data_);

  gl::Bind(normal_tex_buffer_);
  normal_tex_buffer_.data(last_data_alloc_ * sizeof(DerivativeInfo), nullptr, gl::kDynamicDraw);
  normal_tex_buffer_.subData(0, normal_data_);
}

void TexQuadTree::uploadNewData(size_t last_data_size) {
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

void TexQuadTree::imageLoaderThread() {
  while (!worker_should_quit_) {
    // wait until there's something to process
    std::unique_lock<std::mutex> lock{load_later_ownership_};
    condition_variable_.wait(lock, [this]{
      return worker_should_quit_ ||
        (!load_later_.empty() && !worker_thread_should_sleep_);
    });

    if (worker_should_quit_) { return; }

    // process one element of load later
    auto iter = load_later_.begin();
    // (*iter)->upload(height_data_, normal_data_, index_data_);
    (*iter)->load();
    load_later_.erase(iter);
  }
}

TexQuadTree::TexQuadTree(int w /*= GlobalHeightMap::tex_w*/,
                         int h /*= GlobalHeightMap::tex_h*/,
                        glm::ivec2 min_node_size /*= {256, 128}*/)
    : min_node_size_{min_node_size}
    , max_node_level_(max_node_level(w, h))
    , root_{w/2, h/2, w, h, max_node_level_, 0}
    , worker_{[this]{imageLoaderThread();}} {
  initTexIndexBuffer();
  initTextures();
}

TexQuadTree::TexQuadTree(int w, int h, GLubyte max_depth)
    : min_node_size_{w >> max_depth, h >> max_depth}
    , max_node_level_(max_depth)
    , root_{w/2, h/2, w, h, max_node_level_, 0}
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

    size_t last_data_size = height_data_.size();
    bool is_first_call = (update_counter_ == 0);

    // the cached data should expire once in a while
    if (++update_counter_ % 1000 == 0) {
      last_data_size = 0;
      std::memset(index_data_.data(), 0,
                  index_data_.size() * sizeof(index_data_[0]));
      height_data_.clear();
      normal_data_.clear();
    }

    glm::vec3 cam_pos = cam.transform()->pos();

    // Forget load_later data that we couldn't load since last frame. If some
    // texture wasn't loaded by the worker thread, but it is still needed, then
    // the selectNodes will add it again. If it won't add it - then it's not
    // required anymore to render, so it's a good thing that we dropped it.
    if (!load_later_.empty()) {
      load_later_.clear();
    }
    root_.selectNodes(cam_pos, cam.frustum(), height_data_, normal_data_,
                      index_data_, load_later_, is_first_call);

    // unload unused textures, and keep track of last use times
    root_.age();

    // upload the data
    gl::Bind(index_tex_buffer_);
    index_tex_buffer_.subData(0, index_data_);
    gl::Unbind(index_tex_buffer_);

    size_t new_data_size = height_data_.size();
    if (new_data_size > last_data_alloc_) {
      enlargeBuffers (new_data_size);
    } else {
      uploadNewData (last_data_size);
    }
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
