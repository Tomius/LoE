// Copyright (c) 2014, Tamas Csala

#ifndef ENGINE_CDLOD_QUAD_TREE_H_
#define ENGINE_CDLOD_QUAD_TREE_H_

#include <memory>
#include "./quad_grid_mesh.h"
#include "../camera.h"
#include "../collision/bounding_box.h"
#include "../collision/bounding_spherical_sector.h"
#include "../height_map_interface.h"
#include "../../oglwrap/debug/insertion.h"

namespace engine {
namespace cdlod {

class QuadTree {
  QuadGridMesh mesh_;
  int node_dimension_;

  struct Node {
    GLshort x, z;
    BoundingSphericalSector bbox;
    GLushort size;
    GLubyte level;
    std::unique_ptr<Node> tl, tr, bl, br;

    Node(GLshort x, GLshort z, GLubyte level, int dimension, bool root = false);

    // Helper to create the quadtree in four threads
    static void Init(GLshort x, GLshort z, GLubyte level,
                     int dimension, std::unique_ptr<Node>* node);

    bool collidesWithSphere(const glm::vec3& center, float radius) {
      return bbox.collidesWithSphere(center, radius);
    }

    void countMinMaxOfArea(const HeightMapInterface& hmap,
                           double *min, double *max, bool root = false);

    // Helper to run countMinMaxOfArea in thread
    static void CountMinMaxOfArea(Node* node, const HeightMapInterface& hmap,
                                  double *min, double *max) {
      node->countMinMaxOfArea(hmap, min, max);
    }

    void selectNodes(const glm::vec3& cam_pos, const Frustum& frustum,
                     QuadGridMesh& grid_mesh, int node_dimension);
  };

  Node root_;

 public:
  QuadTree(const HeightMapInterface& hmap, int node_dimension = 128)
      : mesh_(node_dimension), node_dimension_(node_dimension)
      , root_(hmap.w()/2, hmap.h()/2,
        std::max(ceil(log2(std::max(hmap.w(), hmap.h())) - log2(node_dimension)), 0.0),
        node_dimension, true) {
    double min, max;
    root_.countMinMaxOfArea(hmap, &min, &max, true);
  }

  int node_dimension() const {
    return node_dimension_;
  }

  void setupPositions(gl::VertexAttrib attrib) {
    mesh_.setupPositions(attrib);
  }

  void setupRenderData(gl::VertexAttrib attrib) {
    mesh_.setupRenderData(attrib);
  }

  // static glm::vec3 cartesianToSpherical(glm::vec3 const& v) {
  //   glm::vec3 ret;
  //   ret.y = glm::length(v);
  //   ret.x = atan2(v.y, v.x);
  //   ret.z = atan2(glm::length(glm::vec2(v.y, v.x)), v.z);
  //   return ret;
  // }

  // static glm::vec3 globalToPlanar(glm::vec3 const& v) {
  //   glm::vec3 ret = cartesianToSpherical(v);
  //   ret.y -= 10000;
  //   // to degree
  //   ret.x *= 180 / 3.14159265359;
  //   ret.z *= 180 / 3.14159265359;
  //   // to tex coords
  //   ret.x *= 5400*4/360;
  //   ret.z *= 2700*4/180;
  //   return ret;
  // }

  void render(const engine::Camera& cam) {
    mesh_.clearRenderList();
    glm::vec3 cam_pos = cam.transform()->pos();
    root_.selectNodes(cam_pos, cam.frustum(), mesh_, node_dimension_);
    mesh_.render();
  }
};

}  // namespace cdlod
}  // namespace engine

#endif
