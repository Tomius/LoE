// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_
#define ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_

#include "./bounding_box.h"
#include "../global_height_map.h"

namespace engine {

template<size_t max_w, size_t max_h>
class BoundingSphericalSector : public BoundingBox {
  bool invalid_ = true;

  static bool isValid(glm::dvec3 model_pos) {
    return 0 <= model_pos.x && model_pos.x <= max_w &&
           0 <= model_pos.z && model_pos.z <= max_h;
  }

  static glm::dvec3 transform(glm::dvec3 model_pos) {
    glm::dvec2 angles_degree{360.0 * (model_pos.x / max_w),
                             180.0 * (model_pos.z / max_h)};
    angles_degree = glm::dvec2(angles_degree.x, angles_degree.y);
    glm::dvec2 angles = angles_degree * double(M_PI / 180);
    double r = GlobalHeightMap::sphere_radius + model_pos.y;
    glm::dvec3 cartesian = glm::dvec3(
      r*sin(angles.y)*cos(angles.x),
      r*cos(angles.y),
      -r*sin(angles.y)*sin(angles.x)
    );

    return cartesian;
  }

 public:
  BoundingSphericalSector() = default;

  BoundingSphericalSector(const glm::dvec3& mins, const glm::dvec3& maxes) {
    glm::dvec3 diff = maxes - mins;
    glm::dvec3 step = glm::min(diff / 4.001, 2048.0);
    for (double x = mins.x; x < maxes.x; x += step.x) {
      for (double y = mins.y; y < maxes.y; y += step.y) {
        for (double z = mins.z; z < maxes.z; z += step.z) {
          glm::dvec3 vec{x, y, z};
          if (isValid(vec)) {
            glm::dvec3 tvec{transform(vec)};
            if (invalid_) {
              mins_ = tvec;
              maxes_ = tvec;
              invalid_ = false;
            } else {
              mins_ = glm::min(tvec, mins_);
              maxes_ = glm::max(tvec, maxes_);
            }
          }
        }
      }
    }
  }

  virtual bool collidesWithSphere(const glm::dvec3& center, double radius) const {
    if (invalid_) {
      return false;
    } else {
      return BoundingBox::collidesWithSphere(center, radius);
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
