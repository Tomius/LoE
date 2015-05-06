// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_SPHERE_H_
#define ENGINE_COLLISION_SPHERE_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "../misc.h"
#include "./frustum.h"

namespace engine {

class Sphere {
 protected:
  glm::dvec3 center_;
  double radius_ = 0.0;

 public:
  Sphere() = default;

  Sphere(glm::dvec3 const& center, double radius)
      : center_(center), radius_(radius) {}

  glm::dvec3 center() const { return center_; }
  double radius() const { return radius_; }

  virtual bool collidesWithSphere(const Sphere& sphere) const {
    double dist = glm::length(center_ - sphere.center_);
    return dist < radius_ + sphere.radius_;
  }

  virtual bool collidesWithFrustum(const Frustum& frustum) const {
    // calculate our distances to each of the planes
    for (int i = 0; i < 6; ++i) {
      const Plane& plane = frustum.planes[i];

      // find the distance to this plane
      double dist = glm::dot(plane.normal, center_) + plane.dist;

      // if this distance is < -sphere.radius, we are outside
      if (dist < -radius_)
        return false;

      // else if the distance is between +- radius, then we intersect
      if (std::abs(dist) < radius_)
        return true;
    }

    // otherwise we are fully in view
    return true;
  }
};

}


#endif
