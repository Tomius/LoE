// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_GLOBAL_HEGIHT_MAP_H_
#define ENGINE_GLOBAL_HEGIHT_MAP_H_

#include <climits>
#include "./transform.h"
#include "./texture_source.h"

namespace engine {

namespace GlobalHeightMap {
  // the path of the directory where the heightmap lies
  extern const char *height_texture_base_path;
  extern const char *dx_texture_base_path;
  extern const char *dy_texture_base_path;

  static constexpr int level_offset = 0;

  // CDLOD nodes' extent is (1 << node_dimension_exp)
  static constexpr int node_dimension_exp = 5;
  static_assert(4 <= node_dimension_exp && node_dimension_exp <= 8, "");

  static constexpr int node_dimension = 1 << node_dimension_exp;

  // The size of sphere for a CDLOD level is node size * this
  // It should be at least 2, but making it bigger makes distant
  // parts of the terrain appear with more detail.
  static constexpr double lod_level_distance_multiplier = 3.0;
  static_assert(1 <= lod_level_distance_multiplier, "");

  static constexpr double texture_level_distance_multiplier = 0.75;

  // Geometry subdivision. This practially contols zooming into the heightmap.
  // If for ex. this is three, that means that a 8x8 geometry (9x9 vertices)
  // corresponds to a 1x1 texture area (2x2 texels)
  static constexpr long geom_div_base = 2;
  static_assert(geom_div_base <= 2*node_dimension_exp, "");

  static constexpr long geom_div = geom_div_base - level_offset;

  // The resolution of the heightmap
  static constexpr long tex_w = 172800, tex_h = 86400;

  // The radius of the sphere made of the heightmap
  static constexpr double sphere_radius = tex_w / 2 / M_PI;

  static constexpr int mt_everest_height = 8848 * (sphere_radius / 6371000);
  static constexpr int height_scale = 5;
  static constexpr int max_height = height_scale * mt_everest_height;

  // statistics
  extern size_t geom_nodes_count, texture_nodes_count, gpu_mem_usage;
};

}  // namespace engine

#endif
