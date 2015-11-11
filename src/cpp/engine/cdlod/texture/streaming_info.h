// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_STREAMING_INFO_H_
#define ENGINE_CDLOD_TEXTURE_STREAMING_INFO_H_

#include "../../oglwrap_all.h"

namespace engine {
namespace cdlod {

class TexQuadTreeNode;

struct TexQuadTreeNodeIndex {
  GLushort data_offset_hi = 0, data_offset_lo = 0;
  GLushort tex_size_x = 0, tex_size_y = 0;
};

struct DerivativeInfo {
  GLushort dx = 0, dy = 0;
};
static_assert(sizeof(DerivativeInfo) == 2*sizeof(GLushort), "");

using HeightData = GLushort;

struct TextureInfo {
  GLushort tex_size_x = 0, tex_size_y = 0;
  GLushort child0_hi  = 0, child0_lo  = 0;
  GLushort child1_hi  = 0, child1_lo  = 0;
  GLushort child2_hi  = 0, child2_lo  = 0;
  GLushort child3_hi  = 0, child3_lo  = 0;
};

struct TexelData {
  GLushort height = 0, dx = 0, dy = 0;
};

struct StreamingInfo {
  size_t last_data_alloc = 16*1024*1024;
  size_t uploaded_texel_count = 0;

  gl::TextureBuffer height_tex_buffer;
  gl::TextureBuffer normal_tex_buffer;
  gl::TextureBuffer index_tex_buffer;
  gl::BufferObject<gl::BufferType::kCopyWriteBuffer> copyBuffer;

  std::vector<TexQuadTreeNodeIndex> index_data;
  std::vector<TexQuadTreeNode*> data_owners;
};

}
}

#endif
