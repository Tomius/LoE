// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_
#define ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "../misc.h"
#include "./frustum.h"
#include "../global_terrain_setting.h"
#include "../../oglwrap/debug/insertion.h"

namespace engine {

class BoundingSphericalSector : public BoundingBox {

  static glm::vec3 transform(glm::vec3 model_pos) {
    glm::vec2 angles_degree{model_pos.x * 360 / global_terrain::w,
                            model_pos.z * 180 / global_terrain::h};
    angles_degree = glm::vec2(360-angles_degree.x, 180-angles_degree.y);
    glm::vec2 angles = 1.001f * angles_degree * float(M_PI / 180);
    float r = global_terrain::sphere_radius + model_pos.y;
    glm::vec3 cartesian = glm::vec3(
      r*sin(angles.y)*cos(angles.x),
      r*sin(angles.y)*sin(angles.x),
      r*cos(angles.y)
    );

    return cartesian;
  }

 public:
  BoundingSphericalSector() = default;

  BoundingSphericalSector(const glm::vec3& mins, const glm::vec3& maxes) {
    glm::vec3 diff = maxes - mins;
    mins_ = maxes_ = transform(maxes);
    for (float x = mins.x; x < maxes.x; x += diff.x/2.01f) {
      for (float y = mins.y; y < maxes.y; y += diff.y/2.01f) {
        for (float z = mins.z; z < maxes.z; z += diff.z/2.01f) {
          mins_ = glm::min(transform({x, y, z}), mins_);
          maxes_ = glm::max(transform({x, y, z}), maxes_);
        }
      }
    }
  }
};

}


#endif
