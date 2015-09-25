// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_GLOBAL_HEGIHT_MAP_H_
#define ENGINE_GLOBAL_HEGIHT_MAP_H_

#include <climits>
#include "./transform.h"
#include "./texture_source.h"

namespace engine {

namespace GlobalHeightMap {
  // the path of the directory where the heightmap lies
  extern const char *base_path;

  // CDLOD nodes' extent is (1 << node_dimension_exp)
  static constexpr uint node_dimension_exp = 5;
  static_assert(4 <= node_dimension_exp && node_dimension_exp <= 8, "");

  // The size of sphere for a CDLOD level is node size * this
  // It should be at least 2, but making it bigger makes distant
  // parts of the terrain appear with more detail.
  static constexpr float lod_level_distance_multiplier = 2.0;
  static_assert(2 <= lod_level_distance_multiplier, "");

  // Geometry subdivision. This practially contols zooming into the heightmap.
  // If for ex. this is three, that means that a 8x8 geometry (9x9 vertices)
  // corresponds to a 1x1 texture area (2x2 texels)
  static constexpr long geom_div = 4;

  static_assert(geom_div < node_dimension_exp, "");

  // The resolution of the heightmap
  static constexpr long tex_w = 172800, tex_h = 86400;

  // The resolution of the geometry subdivided heightmap.
  static constexpr long geom_w = tex_w << geom_div;
  static constexpr long geom_h = tex_h << geom_div;

  // The maximum height value of a "hill"
  static constexpr int max_height = 100 << geom_div;

  // The radius of the sphere made of the heightmap
  static constexpr float sphere_radius = geom_w / 2 / M_PI;
};

}  // namespace engine

#endif
