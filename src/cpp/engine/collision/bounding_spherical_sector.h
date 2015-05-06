// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_
#define ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_

#include "./bounding_box.h"
#include "../global_height_map.h"

namespace engine {

template<size_t max_w, size_t max_h>
class BoundingSphericalSector : public BoundingBox {
  bool invalid_ = true;

 public:
  BoundingSphericalSector() = default;

  BoundingSphericalSector(const glm::dvec3& mins, const glm::dvec3& maxes) {
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

}


#endif
