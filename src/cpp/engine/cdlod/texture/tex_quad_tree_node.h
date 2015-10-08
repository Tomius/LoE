// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_
#define ENGINE_CDLOD_TEXTURE_TEX_QUAD_TREE_NODE_H_

#include <memory>
#include <array>

namespace engine {
namespace cdlod {

constexpr const int kTerrainWidth = 172800;
constexpr const int kTerrainHeight = 86400;

class TexQuadTreeNode {
 public:
  TexQuadTreeNode(int center_x, int center_z,
                  int size_x, int size_z,
                  GLubyte mip_level);

  void generateImage() const;
  void setSiblings(const std::array<TexQuadTreeNode*, 8>& siblings);

 private:
  int x_, z_, sx_, sz_;
  GLubyte level_;
  std::unique_ptr<TexQuadTreeNode> children_[4];
  std::array<TexQuadTreeNode*, 8> siblings_;

  void initChildren();
  void createImage() const;
  bool isImageReady() const;
  TexQuadTreeNode* get4x4OwnOrSiblingChildren(int x, int y) const;
};

}
}

#endif
