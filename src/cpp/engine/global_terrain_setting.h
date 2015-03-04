#ifndef GLOBAL_TERRAIN_SETTING_H_
#define GLOBAL_TERRAIN_SETTING_H_

namespace global_terrain {
  static const std::string base_path = "src/resources/gmted2010/1/";
  constexpr size_t w = 5400*4, h = 2700*4;
  constexpr size_t atlas_w = 12, atlas_h = 9;
  constexpr size_t atlas_elem_num = atlas_w*atlas_h;
  constexpr size_t atlas_elem_w = w / atlas_w;
  constexpr size_t atlas_elem_h = h / atlas_h;
  constexpr float sphere_radius = w / 2 / M_PI;
}

#endif
