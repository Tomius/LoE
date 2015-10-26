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

struct StreamingInfo {
  size_t last_data_alloc = 64*1024*1024;
  size_t uploaded_texel_count = 0;
  gl::TextureBuffer height_tex_buffer;
  gl::TextureBuffer normal_tex_buffer;
  gl::TextureBuffer index_tex_buffer;
  std::vector<TexQuadTreeNodeIndex> index_data;
  std::vector<TexQuadTreeNode*> data_owners;
};

}
}

#endif
