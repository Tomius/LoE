#include "global_height_map.h"

namespace engine {

const char *GlobalHeightMap::base_path = "src/resources/gmted2010_75/";
//size_t GlobalHeightMap::level_offset = 0;
size_t GlobalHeightMap::w = 172800;
size_t GlobalHeightMap::h = 86400;
float GlobalHeightMap::sphere_radius = w / 2 / M_PI;

}
