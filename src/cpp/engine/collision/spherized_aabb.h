// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_SPHERIZED_AABB_H_
#define ENGINE_COLLISION_SPHERIZED_AABB_H_

#include "../misc.h"
#include "./bounding_box.h"
#include "../global_height_map.h"

namespace engine {

static constexpr double kEpsilon = 1e-5;

struct Interval {
  double min;
  double max;
};

static inline bool HasIntersection(Interval a, Interval b) {
  return a.min + kEpsilon < b.max && b.min + kEpsilon < a.max;
}

template<size_t max_w, size_t max_h>
class SpherizedAABB : public BoundingBox {
  bool invalid_ = true;

 public:
  SpherizedAABB() = default;

  SpherizedAABB(const glm::dvec3& mins, const glm::dvec3& maxes) {
    using namespace glm;

    if (max_w < mins.x || max_h < mins.z || maxes.x < 0 || maxes.z < 0) {
      invalid_ = true;
      return;
    } else {
      invalid_ = false;
    }

    dvec2 angle_mins_degree{360.0 * (mins.x / max_w), 180.0 * (mins.z / max_h)};
    dvec2 angle_mins = angle_mins_degree * double(M_PI / 180);

    dvec2 angle_maxes_degree{360.0 * (maxes.x / max_w), 180.0 * (maxes.z / max_h)};
    dvec2 angle_maxes = angle_maxes_degree * double(M_PI / 180);

    dvec2 s1 = sin(angle_mins), s2 = sin(angle_maxes);
    dvec2 max_s = max(s1, s2), min_s = min(s1, s2);

    dvec2 c1 = cos(angle_mins), c2 = cos(angle_maxes);
    dvec2 max_c = max(c1, c2), min_c = min(c1, c2);

    for (int i = 0; i < 2; ++i) {
      if (angle_mins_degree[i] < 0 && 0 < angle_maxes_degree[i])
        max_c[i] = 1;
      if (angle_mins_degree[i] < 90 && 90 < angle_maxes_degree[i])
        max_s[i] = 1;
      if (angle_mins_degree[i] < 180 && 180 < angle_maxes_degree[i])
        min_c[i] = -1;
      if (angle_mins_degree[i] < 270 && 270 < angle_maxes_degree[i])
        min_s[i] = -1;
    }

    dvec2 sx_bounds{min_s.x, max_s.x}, sy_bounds{min_s.y, max_s.y},
          cx_bounds{min_c.x, max_c.x}, cy_bounds{min_c.y, max_c.y},
          r_bounds{GlobalHeightMap::sphere_radius + mins.y,
                   GlobalHeightMap::sphere_radius + maxes.y};

    for (int r = 0; r < 2; ++r) {
      for (int sy = 0; sy < 2; ++sy) {
        for (int cx = 0; cx < 2; ++cx) {
          double x = r_bounds[r] * sy_bounds[sy] * cx_bounds[cx];
          if ((!r && !sy && !cx) || x < mins_.x) mins_.x = x;
          if ((!r && !sy && !cx) || maxes_.x < x) maxes_.x = x;
        }
        for (int sx = 0; sx < 2; ++sx) {
          double z = -r_bounds[r] * sy_bounds[sy] * sx_bounds[sx];
          if ((!r && !sy && !sx) || z < mins_.z) mins_.z = z;
          if ((!r && !sy && !sx) || maxes_.z < z) maxes_.z = z;
        }
      }
      for (int cy = 0; cy < 2; ++cy) {
        double y = r_bounds[r] * cy_bounds[cy];
        if ((!r && !cy) || y < mins_.y) mins_.y = y;
        if ((!r && !cy) || maxes_.y < y) maxes_.y = y;
      }
    }
  }

  virtual bool collidesWithSphere(const Sphere& sphere) const {
    if (invalid_) {
      return false;
    } else {
      return BoundingBox::collidesWithSphere(sphere);
    }
  }

  virtual bool collidesWithFrustum(const Frustum& frustum) const {
    if (invalid_) {
      return false;
    } else {
      return BoundingBox::collidesWithFrustum(frustum);
    }
  }
};

template<size_t max_w, size_t max_h>
static inline glm::dvec3 Model2WorldPos(glm::dvec3 model_pos) {
  glm::dvec2 angles = glm::dvec2(model_pos.x / max_w * 2 * M_PI, model_pos.z / max_h * M_PI);
  double r = GlobalHeightMap::sphere_radius + model_pos.y;
  glm::dvec3 cartesian = glm::dvec3(
    r*sin(angles.y)*cos(angles.x),
    r*cos(angles.y),
    -r*sin(angles.y)*sin(angles.x)
  );

  return cartesian;
}

template<size_t max_w, size_t max_h>
class SpherizedAABBSat {
  bool invalid_ = true;
  bool use_aabb = false;
  SpherizedAABB<max_w, max_h> aabb_;
  glm::vec2 percent_;

  glm::dvec3 normals_[4];
  Interval extents_[4];
  Interval radial_extent_;

 public:
  SpherizedAABBSat() = default;

  SpherizedAABBSat(glm::dvec3 mins, glm::dvec3 maxes) {
    using namespace glm;

    if (max_w < mins.x || max_h < mins.z || maxes.x < 0 || maxes.z < 0) {
      invalid_ = true;
      return;
    } else {
      invalid_ = false;
    }

    // clamp to edge
    mins.x = std::max<double>(mins.x, 0);
    mins.z = std::max<double>(mins.z, 0);
    maxes.x = std::min<double>(maxes.x, max_w);
    maxes.z = std::min<double>(maxes.z, max_h);
    aabb_ = {mins, maxes};

    percent_ = {(maxes.x - mins.x) * 100 / max_w,
                (maxes.z - mins.z) * 100 / max_h};
    if (percent_.x >= 100/16.0 || percent_.y >= 100/16.0) {
      use_aabb = true;
      return;
    }

    double radius = GlobalHeightMap::sphere_radius;
    radial_extent_ = {radius + mins.y, radius + maxes.y};

    /*
      The input coordinate system is right handed:
          (O)----(x - longite)
          /|
         / |
 (y - rad) |
           |
      (z - latitude)

      But we can use any convenient coordinate system,
      as long as it is right handed too:

          (y)
           |
           |
           |
          (O)-----(x)
          /
         /
       (z)

      The verices:

           (E)-----(A)
           /|      /|
          / |     / |
        (F)-----(B) |
         | (H)---|-(D)
         | /     | /
         |/      |/
        (G)-----(C)
    */

    enum {
      A, B, C, D, E, F, G, H
    };

    glm::dvec3 vertices[8];
    vertices[A] = {maxes.x, maxes.y, mins.z};
    vertices[B] = {maxes.x, maxes.y, maxes.z};
    vertices[C] = {maxes.x, mins.y,  maxes.z};
    vertices[D] = {maxes.x, mins.y,  mins.z};
    vertices[E] = {mins.x,  maxes.y, mins.z};
    vertices[F] = {mins.x,  maxes.y, maxes.z};
    vertices[G] = {mins.x,  mins.y,  maxes.z};
    vertices[H] = {mins.x,  mins.y,  mins.z};

    for (glm::dvec3& vert : vertices) {
      vert = Model2WorldPos<max_w, max_h>(vert);
    }

    enum {
      Front, Right, Back, Left
    };

    // normals are towards the inside of the AABB
    normals_[Front] = normalize(cross(vertices[G]-vertices[C], vertices[B]-vertices[C]));
    normals_[Right] = normalize(cross(vertices[C]-vertices[D], vertices[A]-vertices[D]));
    normals_[Back]  = normalize(cross(vertices[D]-vertices[H], vertices[E]-vertices[H]));
    normals_[Left]  = normalize(cross(vertices[H]-vertices[G], vertices[F]-vertices[G]));

    extents_[Front] = {dot(vertices[B], normals_[Front]), dot(vertices[A], normals_[Front])};
    extents_[Right] = {dot(vertices[A], normals_[Right]), dot(vertices[E], normals_[Right])};
    extents_[Back]  = {dot(vertices[E], normals_[Back]),  dot(vertices[F], normals_[Back])};
    extents_[Left]  = {dot(vertices[F], normals_[Left]),  dot(vertices[B], normals_[Left])};

    for (size_t i = 0; i < 4; ++i) {
      if (extents_[i].max + kEpsilon < extents_[i].min) {
        std::cout << "Error" << std::endl;
        abort();
      }
    }
  }

  virtual bool collidesWithSphere(const Sphere& sphere) const {
    if (invalid_) {
      return false;
    } else if (use_aabb) {
      return aabb_.collidesWithSphere (sphere);
    } else {
      for (size_t i = 0; i < 4; ++i) {
        double interval_center = dot(sphere.center(), normals_[i]);
        Interval projection_extent = {interval_center - sphere.radius(),
                                      interval_center + sphere.radius()};

        if (!HasIntersection(extents_[i], projection_extent)) {
          return false;
        }
      }

      double radial_interval_center = length(sphere.center());
      Interval radial_extent = {radial_interval_center - sphere.radius(),
                                radial_interval_center + sphere.radius()};
      if (!HasIntersection(radial_extent_, radial_extent)) {
        return false;
      }

      //std::cout << "Collision " << percent_ << std::endl;
      return true;
    }
  }

  virtual bool collidesWithFrustum(const Frustum& frustum) const {
    if (invalid_) {
      return false;
    } else {
      return aabb_.collidesWithFrustum(frustum);
    }
  }
};


}


#endif
