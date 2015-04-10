#include "global_height_map.h"

namespace engine {

const char *GlobalHeightMap::base_path = "src/resources/gmted2010/1/";
size_t GlobalHeightMap::w = 5400*4, GlobalHeightMap::h = 2700*4;
size_t GlobalHeightMap::atlas_w = 12, GlobalHeightMap::atlas_h = 9;
size_t GlobalHeightMap::atlas_elem_count = atlas_w*atlas_h;
size_t GlobalHeightMap::atlas_elem_w = w / atlas_w;
size_t GlobalHeightMap::atlas_elem_h = h / atlas_h;
float GlobalHeightMap::sphere_radius = w / 2 / M_PI;

}
