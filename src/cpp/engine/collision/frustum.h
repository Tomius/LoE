// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_COLLISION_FRUSTUM_H_
#define ENGINE_COLLISION_FRUSTUM_H_

#include "plane.h"

struct Frustum {
  Plane planes[6]; // left, right, top, down, near, far
};

#endif
