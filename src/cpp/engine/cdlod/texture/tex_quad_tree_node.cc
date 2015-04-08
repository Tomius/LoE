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

// If a node is not used for this much time (frames), it will be unloaded.
int TexQuadTreeNode::time_to_live_ = 256;

static Magick::Image load(std::string const& dir, int x, int y) {
  char str[30];
  sprintf(str, "%d/%d_%d.jpg", x, x, y);
  return Magick::Image{dir + str};
}

static void createImage0(int x, int y, size_t sx, size_t sy) {
  using namespace Magick;

  // convert coordinates to top-left
  x -= sx/2;
  y -= sy/2;

  char str[30];
  sprintf(str, "%d.jpg", y);

  const std::string dir = "src/resources/jpg_very_small/";
  std::string out_dir =
    "src/resources/gmted2010_75/0/" + std::to_string(x) + "/";
  const int w = 450, h = 300;
  assert(w > sx && h > sy);

  std::cout << "Creating: (0, " << x << ", " << y << ")" << std::endl;
  system(("mkdir -p " + out_dir).c_str());

  // top x, y
  int tx = x - x%w, ty = y - y%h;
  // space left x, y
  int slx = w - x%w, sly = h - y%h;

  // near the edges we need to load more than one image
  if (slx < sx && sly < sy) {
    Image im0 = load(dir, tx, ty);
    Image im1 = load(dir, tx+w, ty);
    Image im2 = load(dir, tx, ty+h);
    Image im3 = load(dir, tx+w, ty+h);

    Image image{Geometry{2*w, 2*h}, ColorRGB{}};
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, w, 0, CopyCompositeOp);
    image.composite(im2, 0, h, CopyCompositeOp);
    image.composite(im3, w, h, CopyCompositeOp);
    image.crop(Geometry{sx, sy, x-tx, y-ty});

    image.write(out_dir + str);
  } else if(slx < sx) {
    Image im0 = load(dir, tx, ty);
    Image im1 = load(dir, tx+w, ty);

    Image image{Geometry{2*w, h}, ColorRGB{}};
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, w, 0, CopyCompositeOp);
    image.crop(Geometry{sx, sy, x-tx, y-ty});

    image.write(out_dir + str);
  } else if(sly < sy) {
    Image im0 = load(dir, tx, ty);
    Image im1 = load(dir, tx, ty+h);

    Image image{Geometry{w, 2*h}, ColorRGB{}};
    image.composite(im0, 0, 0, CopyCompositeOp);
    image.composite(im1, 0, h, CopyCompositeOp);
    image.crop(Geometry{sx, sy, x-tx, y-ty});

    image.write(out_dir + str);
  } else {
    Image image = load(dir, tx, ty);
    image.crop(Geometry{sx, sy, x-tx, y-ty});

    image.write(out_dir + str);
  }
}

bool TexQuadTreeNode::isImageReady() {
  using namespace Magick;

  int tx = x_ - sx_/2;
  int ty = z_ - sz_/2;
  char full_path[80];
  const char* base_path = "src/resources/gmted2010_75";
  sprintf(full_path, "%s/%d/%d/%d.jpg", base_path, level_, tx, ty);

  try {
    Image{full_path};
    return true;
  } catch(...) {
    return false;
  }
}

void TexQuadTreeNode::createImage() {
  using namespace Magick;
  const char* base_path = "src/resources/gmted2010_75";

  int tx = x_ - sx_/2;
  int ty = z_ - sz_/2;
  char dir[50], filename[30];
  sprintf(dir, "%s/%d/%d/", base_path, level_, tx);
  sprintf(filename, "%d.jpg", ty);
  std::string full_path = std::string{dir} + filename;

  std::cout << "Creating: (" << (int)level_ << ", " << tx << ", " << ty << ")" << std::endl;
  system(("mkdir -p " + std::string{dir}).c_str());

  Image child_im[4];
  for (int i = 0; i < 4; ++i) {
    auto& child = children_[i];
    assert(child);

    int clevel = child->level_;
    int ctx = child->x_ - child->sx_/2;
    int cty = child->z_ - child->sz_/2;
    char str[80];
    sprintf(str, "%s/%d/%d/%d.jpg", base_path, clevel, ctx, cty);
    child_im[i].read(str);
  }

  int im_w = child_im[0].size().width() + child_im[1].size().width();
  int im_h = child_im[0].size().height() + child_im[2].size().height();
  Image image{Geometry(im_w, im_h), ColorRGB{}};
  for (int i = 0; i < 4; ++i) {
    auto& child = children_[i];
    int ctx = child->x_ - child->sx_/2;
    int cty = child->z_ - child->sz_/2;
    auto& c_im = child_im[i];

    int maskx = ctx - tx > 0 ? 1 : 0;
    int masky = cty - ty > 0 ? 1 : 0;
    image.composite(c_im, maskx*c_im.size().width(),
                    masky*c_im.size().height(), CopyCompositeOp);
  }

  image.resize(Geometry(im_w/2, im_h/2));
  image.write(full_path);
}

TexQuadTreeNode::TexQuadTreeNode(int x, int z, int sx, int sz, GLubyte level)
    : x_(x), z_(z), sx_(sx), sz_(sz), level_(level)
    /*, bbox_{{x-sx/2, 0, z-sz/2}, {x+(sx-sx/2), 100, z+(sz-sz/2)}}*/ {
  if (!isImageReady()) {
    if (level_ != 0) {
      for (int i = 0; i < 4; ++i) {
        initChild(i);
      }
      createImage();
    } else {
      createImage0(x, z, sx, sz);
    }
  }
}



// TODO
// it just decodes a jpg files to examine performance
void TexQuadTreeNode::load() {
  if (data_) {
    return;
  }

  // if (level_ == 0) {
  //   std::cout << sx_ << ", " << sz_ << std::endl;
  // }

  std::string path = "src/resources/gmted2010/4/000_000.jpg";
  Magick::Image image(path);
  size_t w = image.columns();
  size_t h = image.rows();
  data_ = std::unique_ptr<GLubyte>{new GLubyte[w*h]};
  image.write(0, 0, w, h, "R", MagickCore::CharPixel, data_.get());
}

void TexQuadTreeNode::age() {
  last_used_++;

  for (auto& child : children_) {
    if (child) {
      // unload child if its age would exceed the ttl
      if (child->last_used_ >= time_to_live_) {
        child.reset();
      } else {
        child->age();
      }
    }
  }
}

void TexQuadTreeNode::initChild(int i) {
  assert(level_ > 0);

  switch (i) {
    case 0: { // top left
      int sx = sx_/2, sz = sz_/2;
      children_[0] = make_unique<TexQuadTreeNode>(
          x_ - (sx - sx/2), z_ - (sz - sz/2), sx, sz, level_-1);
    } break;
    case 1: { // top right
      int sx = sx_ - sx_/2, sz = sz_/2;
      children_[1] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ - (sz - sz/2), sx, sz, level_-1);
    } break;
    case 2: { // bottom left
      int sx = sx_/2, sz = sz_ - sz_/2;
      children_[2] = make_unique<TexQuadTreeNode>(
          x_ - (sx - sx/2), z_ + sz/2, sx, sz, level_-1);
    } break;
    case 3: { // bottom right
      int sx = sx_ - sx_/2, sz = sz_ - sz_/2;
      children_[3] = make_unique<TexQuadTreeNode>(
          x_ + sx/2, z_ + sz/2, sx, sz, level_-1);
    } break;
    default: {
      throw new std::out_of_range("Tried to index "
          + std::to_string(i) + "th child of a quadtree node.");
    }
  }
}

void TexQuadTreeNode::selectNodes(const glm::vec3& cam_pos,
                                  const Frustum& frustum) {
  float lod_range = 1.01 * std::max(sx_, sz_);

  // check if the node is visible
  // if (!bbox_.collidesWithFrustum(frustum)) {
  //   return;
  // }

  // if we can cover the whole area or if we are a leaf
  if (!bbox_.collidesWithSphere(cam_pos, lod_range) || level_ == 0) {
    load(); // load in the texture
  } else {
    bool children_cover_whole_area = true;
    for (int i = 0; i < 4; ++i) {
      auto& child = children_[i];

      if (!child) {
        initChild(i);
      }

      // call selectNodes on the child (recursive)
      if (child->collidesWithSphere(cam_pos, lod_range)) {
        child->selectNodes(cam_pos, frustum);
      } else {
        children_cover_whole_area = false;
      }
    }

    // If we have to render something, we have to load the texture too.
    if (!children_cover_whole_area) {
      load();
    }
  }

  last_used_ = 0;
}

}  // namespace cdlod
}  // namespace engine
