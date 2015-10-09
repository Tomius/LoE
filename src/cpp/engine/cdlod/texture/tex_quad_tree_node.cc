// Copyright (c) 2015, Tamas Csala

#include <algorithm>
#include "./tex_quad_tree_node.h"
#include "../../misc.h"

#include <Magick++.h>
#include <climits>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace engine {
namespace cdlod {

static std::set<const TexQuadTreeNode*> nodes;

static const int kRootLevel = 9;
static int curr_image_count = 0;
static const bool kVerbose = false;
static const int kBorderSize = 2;
static const std::string kBasePath = std::string{"src/resources/gmted2010_75"};
static const int kImageQuality = 75;

double kSobelX[] = {
  4, 3, 2, 1, 0, -1, -2, -3, -4,
  5, 4, 3, 2, 0, -2, -3, -4, -5,
  6, 5, 4, 3, 0, -3, -4, -5, -6,
  7, 6, 5, 4, 0, -4, -5, -6, -7,
  8, 7, 6, 5, 0, -5, -6, -7, -8,
  7, 6, 5, 4, 0, -4, -5, -6, -7,
  6, 5, 4, 3, 0, -3, -4, -5, -6,
  5, 4, 3, 2, 0, -2, -3, -4, -5,
  4, 3, 2, 1, 0, -1, -2, -3, -4,
};

double kSobelY[] = {
   4,  5,  6,  7,  8,  7,  6,  5,  4,
   3,  4,  5,  6,  7,  6,  5,  4,  3,
   2,  3,  4,  5,  6,  5,  4,  3,  2,
   1,  2,  3,  4,  5,  4,  3,  2,  1,
   0,  0,  0,  0,  0,  0,  0,  0,  0,
  -1, -2, -3, -4, -5, -4, -3, -2, -1,
  -2, -3, -4, -5, -6, -5, -4, -3, -2,
  -3, -4, -5, -6, -7, -6, -5, -4, -3,
  -4, -5, -6, -7, -8, -7, -6, -5, -4,
};

static std::string GetPathToImage(std::string const& dir, int x, int y) {
  return dir + std::to_string(x) + '/' + std::to_string(y) + ".png";
}

static void SetupImage(Magick::Image& image) {
  image.matte(false);
  image.quality(kImageQuality);
  image.defineValue("png", "color-type", "0");
  image.defineValue("png", "bit-depth", "16");
}

static Magick::Image EmptyImage(size_t sx, size_t sy) {
  Magick::Image image{Magick::Geometry{sx, sy}, Magick::ColorGray{}};
  SetupImage(image);
  return image;
}

static int GetImageNumToLevel(int level) {
  int q = 4;
  return (pow(q, level+1) - 1) / (q - 1);
}
static const int kAllImageNum = GetImageNumToLevel(kRootLevel);

static int mod(int x, int m) {
    int r = x%m;
    return r<0 ? r+m : r;
}

static std::ostream& PrintPercentString(std::ostream& os) {
  os.precision(6);
  return os << '[' << std::setw(8) << curr_image_count * 100.0 / kAllImageNum << " %]";
}

static std::string GetImageDir(int level, int x) {
  return kBasePath + '/' + std::to_string(level) + '/' + std::to_string(x);
}

static std::string GetImageFilename(int y) {
  return std::to_string(y) + ".png";
}

static std::string GetImagePath(int level, int x, int y) {
  return GetImageDir(level, x) + "/" + GetImageFilename(y);
}

static void PageAndCrop(Magick::Image& im, const Magick::Geometry& geom) {
  im.page(geom);
  im.crop(geom);
}

static void createImage0(int x, int y, size_t sx, size_t sy) {
  using namespace Magick;

  // convert coordinates to top-left
  x -= sx/2;
  y -= sy/2;

  if (kVerbose) {
    PrintPercentString(std::cout) << " Creating: (0, " << x << ", " << y << ")" << std::endl;
  }

  std::string filename = GetImageFilename(y);
  const std::string dir = "/media/icecool/Data/onlab/src/resources/gmted75/png_small/";
  std::string out_dir = GetImageDir(0, x);

  system(("mkdir -p " + out_dir).c_str());

  // add border (after setting the filename and directory)
  sx += 2 * kBorderSize;
  sy += 2 * kBorderSize;
  x -= kBorderSize;
  y -= kBorderSize;

  const int w = 450, h = 300;
  assert(w > sx && h > sy);

  // top x, y
  int tx = mod(x - mod(x, w), kTerrainWidth), ty = mod(y - mod(y, h), kTerrainHeight);

  // space left x, y
  int slx = w - mod(x, w), sly = h - mod(y, h);

  Image image;

  // near the edges we need to load more than one image
  if (slx < sx && sly < sy) {
    Image im0{GetPathToImage(dir, tx, ty)};
    Image im1{GetPathToImage(dir, mod(tx+w, kTerrainWidth), ty)};
    Image im2{GetPathToImage(dir, tx, mod(ty+h, kTerrainHeight))};
    Image im3{GetPathToImage(dir, mod(tx+w, kTerrainWidth), mod(ty+h, kTerrainHeight))};

    image = EmptyImage(2*w, 2*h);
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, w, 0, CopyCompositeOp);
    image.composite(im2, 0, h, CopyCompositeOp);
    image.composite(im3, w, h, CopyCompositeOp);
  } else if(slx < sx) {
    Image im0{GetPathToImage(dir, tx, ty)};
    Image im1{GetPathToImage(dir, mod(tx+w, kTerrainWidth), ty)};

    image = EmptyImage(2*w, h);
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, w, 0, CopyCompositeOp);
  } else if(sly < sy) {
    Image im0{GetPathToImage(dir, tx, ty)};
    Image im1{GetPathToImage(dir, tx, mod(ty+h, kTerrainHeight))};

    image = EmptyImage(w, 2*h);
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, 0, h, CopyCompositeOp);
  } else {
    image.read(GetPathToImage(dir, tx, ty));
    SetupImage(image);
  }

  PageAndCrop(image, Geometry{sx, sy, mod(x, w), mod(y, h)});
  image.write(out_dir + '/' + filename);
}

bool TexQuadTreeNode::isImageReady() const {
  using namespace Magick;

  int tx = x_ - sx_/2;
  int ty = z_ - sz_/2;

  if (kVerbose) {
    PrintPercentString(std::cout) << " Checking if " << tx << ", " << ty << " is valid" << std::endl;
  }

  try {
    Image image;
    image.read(GetImagePath(level_, tx, ty));
    // without this, magick++ won't check image integrity,
    // i.e. corrupt or empty files due to ctrl + c
    image.size();

    if (kVerbose) {
      PrintPercentString(std::cout) << " " << tx << "/" << ty << " is valid" << std::endl;
    }
    return true;
  } catch(...) {
    if (kVerbose) {
      PrintPercentString(std::cout) << " " << tx << "/" << ty << " is invalid" << std::endl;
    }
    return false;
  }
}

void TexQuadTreeNode::createImage() const {
  using namespace Magick;

  int tx = x_ - sx_/2;
  int ty = z_ - sz_/2;
  std::string filename = GetImageFilename(ty);
  std::string out_dir = GetImageDir(level_, tx);
  std::string full_path = out_dir + "/" + filename;

  system(("mkdir -p " + out_dir).c_str());

  Image child_im[4][4];
  glm::ivec2 image_offset[4][4];
  int accum_x = 0;
  int accum_y = 0;
  for (int y = 0; y < 4; ++y) {
    accum_x = 0;
    for (int x = 0; x < 4; ++x) {
      TexQuadTreeNode* child = get4x4OwnOrSiblingChildren(x, y);
      assert(child);

      int clevel = child->level_;
      int csx = child->sx_;
      int csy = child->sz_;
      int ctx = child->x_ - csx/2;
      int cty = child->z_ - csy/2;

      if (kVerbose) {
        PrintPercentString(std::cout) << " Generating dependency image for " << (int)level_ << ", " << tx << ", " << ty << ")" << std::endl;
      }
      child->generateImage();
      if (kVerbose) {
        PrintPercentString(std::cout) << " Loading dependency image for " << (int)level_ << ", " << tx << ", " << ty << ")" << std::endl;
      }
      child_im[y][x].read(GetImagePath(clevel, ctx, cty));
      int w = child_im[y][x].size().width();
      int h = child_im[y][x].size().height();

      // remove the border (not actually neccesary, but keeps the code simpler)
      PageAndCrop(child_im[y][x],
        Geometry(
          w-2*kBorderSize, // sizex
          h-2*kBorderSize, // sizey
          kBorderSize,     // topx
          kBorderSize)     // topy
      );
      image_offset[y][x] = {accum_x, accum_y};

      accum_x += child_im[y][x].size().width();
    }
    accum_y += child_im[y][0].size().height();
  }

  if (kVerbose) {
    PrintPercentString(std::cout) << " Creating: (" << (int)level_ << ", " << tx << ", " << ty << ")" << std::endl;
  }

  // check if offsets are correct
  for (int y = 1; y < 4; ++y) {
    int height = image_offset[y][0].y - image_offset[y-1][0].y;
    for (int x = 1; x < 4; ++x) {
      int current_height = image_offset[y][x].y - image_offset[y-1][x].y;
      assert (height == current_height);
    }
  }
  for (int x = 1; x < 4; ++x) {
    int width = image_offset[0][x].x - image_offset[0][x-1].x;
    for (int y = 1; y < 4; ++y) {
      int current_width = image_offset[y][x].x - image_offset[y][x-1].x;
      assert (width == current_width);
    }
  }

  // paste the image parts onto one big image
  Image image = EmptyImage(accum_x, accum_y);
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      image.composite(child_im[y][x], image_offset[y][x].x,
                      image_offset[y][x].y, CopyCompositeOp);
    }
  }

  // crop the relevant part out of the image
  int composite_width = image_offset[1][3].x - image_offset[1][1].x;
  int composite_height = image_offset[3][1].y - image_offset[1][1].y;
  PageAndCrop(image,
    Geometry(
      // (4 pixels of border is needed to have 2 pixel borders after resize)
      composite_width + 4*kBorderSize,      // sizex
      composite_height + 4*kBorderSize,     // sizey
      image_offset[1][1].x - kBorderSize,   // top x
      image_offset[1][1].y - kBorderSize)   // top y
  );

  // resize the image to "half" size
  int new_width = composite_width/2 + 2*kBorderSize;
  int new_height = composite_height/2 + 2*kBorderSize;

  Geometry new_geom(new_width, new_height);
  new_geom.aspect(true);
  image.resize(new_geom);
  image.write(full_path);
}

TexQuadTreeNode::TexQuadTreeNode(int x, int z, int sx, int sz, GLubyte level)
    : x_(x), z_(z), sx_(sx), sz_(sz), level_(level) {
  initChildren();
  for (auto& sibling : siblings_) {
    sibling = nullptr;
  }
}

void TexQuadTreeNode::generateImage() const
{
  bool already_processed = nodes.find(this) != nodes.end();
  if (!already_processed) {
    if (!isImageReady()) {
      if (level_ != 0) {
        createImage();
      } else {
        createImage0(x_, z_, sx_, sz_);
      }

      curr_image_count++;
      nodes.insert(this);

      if (!kVerbose && curr_image_count % 100 == 0) {
        PrintPercentString(std::cout) << std::endl;
      }
    } else {
      curr_image_count += GetImageNumToLevel(level_);
      nodes.insert(this);
      if (kVerbose) {
        PrintPercentString(std::cout) << " Image ready." << std::endl;
      }
    }
  }
}

void TexQuadTreeNode::initChildren() {
  if (level_ <= 0) {
    return;
  }

  /* Children indexes:
     0 1
     2 3 */

  { // top left
    int sx = sx_/2, sz = sz_/2;
    children_[0] = make_unique<TexQuadTreeNode>(
        x_ - (sx - sx/2), z_ - (sz - sz/2), sx, sz, level_-1);
  }
  { // top right
    int sx = sx_ - sx_/2, sz = sz_/2;
    children_[1] = make_unique<TexQuadTreeNode>(
        x_ + sx/2, z_ - (sz - sz/2), sx, sz, level_-1);
  }
  { // bottom left
    int sx = sx_/2, sz = sz_ - sz_/2;
    children_[2] = make_unique<TexQuadTreeNode>(
        x_ - (sx - sx/2), z_ + sz/2, sx, sz, level_-1);
  }
  { // bottom right
    int sx = sx_ - sx_/2, sz = sz_ - sz_/2;
    children_[3] = make_unique<TexQuadTreeNode>(
        x_ + sx/2, z_ + sz/2, sx, sz, level_-1);
  }
}

void TexQuadTreeNode::setSiblings(const std::array<TexQuadTreeNode*, 8>& siblings) {
  siblings_ = siblings;
  /* Sibling indexes:
     0 1 2
     3 * 4
     5 6 7 */

  if (level_ == 0) {
    return;
  }

  for (auto& child : children_) {
    assert(child != nullptr);
  }

  // ugly, but i don't know better
  children_[0]->setSiblings({{
    /* 0 */ siblings[0]->children_[3].get(),
    /* 1 */ siblings[1]->children_[2].get(),
    /* 2 */ siblings[1]->children_[3].get(),
    /* 3 */ siblings[3]->children_[1].get(),
    /* 4 */ children_[1].get(),
    /* 5 */ siblings[3]->children_[3].get(),
    /* 6 */ children_[2].get(),
    /* 7 */ children_[3].get(),
  }});

  children_[1]->setSiblings({{
    /* 0 */ siblings[1]->children_[2].get(),
    /* 1 */ siblings[1]->children_[3].get(),
    /* 2 */ siblings[2]->children_[2].get(),
    /* 3 */ children_[0].get(),
    /* 4 */ siblings[4]->children_[0].get(),
    /* 5 */ children_[2].get(),
    /* 6 */ children_[3].get(),
    /* 7 */ siblings[4]->children_[2].get(),
  }});

  children_[2]->setSiblings({{
    /* 0 */ siblings[3]->children_[1].get(),
    /* 1 */ children_[0].get(),
    /* 2 */ children_[1].get(),
    /* 3 */ siblings[3]->children_[3].get(),
    /* 4 */ children_[3].get(),
    /* 5 */ siblings[5]->children_[1].get(),
    /* 6 */ siblings[6]->children_[0].get(),
    /* 7 */ siblings[6]->children_[1].get(),
  }});

  children_[3]->setSiblings({{
    /* 0 */ children_[0].get(),
    /* 1 */ children_[1].get(),
    /* 2 */ siblings[4]->children_[0].get(),
    /* 3 */ children_[2].get(),
    /* 4 */ siblings[4]->children_[2].get(),
    /* 5 */ siblings[6]->children_[0].get(),
    /* 6 */ siblings[6]->children_[1].get(),
    /* 7 */ siblings[7]->children_[0].get(),
  }});
}

TexQuadTreeNode* TexQuadTreeNode::get4x4OwnOrSiblingChildren(int x, int y) const {
  for (auto& sibling : siblings_) {
    assert(sibling != nullptr);
  }

  switch (y) { // row
    case 0: switch (x) { // column
        case 0: return siblings_[0]->children_[3].get();
        case 1: return siblings_[1]->children_[2].get();
        case 2: return siblings_[1]->children_[3].get();
        case 3: return siblings_[2]->children_[2].get();
      }
    case 1: switch (x) { // column
        case 0: return siblings_[3]->children_[1].get();
        case 1: return children_[0].get();
        case 2: return children_[1].get();
        case 3: return siblings_[4]->children_[0].get();
      }
    case 2: switch (x) { // column
        case 0: return siblings_[3]->children_[3].get();
        case 1: return children_[2].get();
        case 2: return children_[3].get();
        case 3: return siblings_[4]->children_[2].get();
      }
    case 3: switch (x) { // column
        case 0: return siblings_[5]->children_[1].get();
        case 1: return siblings_[6]->children_[0].get();
        case 2: return siblings_[6]->children_[1].get();
        case 3: return siblings_[7]->children_[0].get();
      }
  }

  assert(false);
  return nullptr;
}

}  // namespace cdlod
}  // namespace engine
