// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_PLANE_H_
#define ENGINE_COLLISION_PLANE_H_

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct Plane {
  glm::dvec3 normal;
  double dist;
  Plane() = default;
  Plane(double nx, double ny, double nz, double dist)
      : normal(nx, ny, nz), dist(dist) { }
  Plane(const glm::dvec3& normal, double dist)
      : normal(normal), dist(dist) { }

  void normalize() {
    double l = glm::length(normal);
    normal /= l;
    dist /= l;
  }
};

#endif
