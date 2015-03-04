// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_
#define ENGINE_COLLISION_BOUNDING_SPHERICAL_SECTOR_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "../misc.h"
#include "./frustum.h"
#include "../global_terrain_setting.h"

namespace engine {

class BoundingSphericalSector {
  glm::vec3 mins_;
  glm::vec3 maxes_;
 public:
  BoundingSphericalSector() = default;

  BoundingSphericalSector(const glm::vec3& mins, const glm::vec3& maxes)
      : mins_(mins), maxes_(maxes) {}

  static glm::vec3 transform(glm::vec3 model_pos) {
    glm::vec2 angles_degree{model_pos.x * 360 / global_terrain::w,
                            model_pos.z * 180 / global_terrain::h};
    //angles_degree = vec2(angles_degree.x, /*180-*/angles_degree.y);
    glm::vec2 angles = 1.001f * angles_degree * float(M_PI / 180);
    float r = global_terrain::sphere_radius + model_pos.y;
    glm::vec3 cartesian = glm::vec3(
      r*sin(angles.y)*cos(angles.x),
      r*sin(angles.y)*sin(angles.x),
      r*cos(angles.y)
    );

    return cartesian;
  }

  bool collidesWithSphere(const glm::vec3& center, float radius) const {
    for (float x = mins_.x; x <= maxes_.x; x += radius/2) {
      //for (float y = mins_.y; y <= maxes_.y; y += radius/2) {
        for (float z = mins_.z; z <= maxes_.z; z += radius/2) {
          glm::vec3 point = transform({x, maxes_.y, z});
          if (length(center - point) < radius) {
            return true;
          }
        }
      //}
    }

    return false;
  }
};

}


#endif
