// Copyright (c) 2014, Tamas Csala

#version 440

#export vec4 CDLODTerrain_modelPos();
#export vec3 CDLODTerrain_worldPos(vec3 model_pos);
#export vec3 CDLODTerrain_worldPos2(vec3 model_pos);
#export vec2 CDLODTerrain_texCoord(vec3 pos);
#export vec3 CDLODTerrain_normal(vec4 pos);

in vec2 CDLODTerrain_aPosition; // I hate the lack of namespaces

// Vertex attrib divisor works like a uniform
in vec4 CDLODTerrain_uRenderData;

vec2 CDLODTerrain_uOffset = CDLODTerrain_uRenderData.xy;
float CDLODTerrain_uScale = CDLODTerrain_uRenderData.z;
int CDLODTerrain_uLevel = int(CDLODTerrain_uRenderData.w);

uniform sampler2DArray CDLODTerrain_uHeightMap;
uniform vec2 CDLODTerrain_uTexSize;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;

vec3 getAtlasTexcoord(vec2 absolute_coord) {
  ivec2 tex_size = ivec2(5400*4, 2700*4);
  ivec2 atlas_size = ivec2(12, 9);
  ivec2 atlas_elem_size = tex_size / atlas_size;
  ivec2 coord = ivec2(absolute_coord);
  ivec2 atlas_elem_coord = coord / atlas_elem_size;
  int index = atlas_elem_coord.y * atlas_size.x + atlas_elem_coord.x;
  return vec3(coord % atlas_elem_size + fract(absolute_coord), index) / vec3(atlas_elem_size, 1);
}

float CDLODTerrain_fetchHeight(vec2 tex_coord, float morph) {
  return textureLod(CDLODTerrain_uHeightMap, getAtlasTexcoord(tex_coord), CDLODTerrain_uLevel + morph).x * 64;
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5) * 2.0;
  return (vertex - frac_part * morph);
}

vec3 CDLODTerrain_worldPos(vec3 model_pos) {
  vec2 angles_degree = model_pos.xz * (vec2(360, 180) / vec2(5400*4, 2700*4));
  angles_degree = vec2(360-angles_degree.x, 180-angles_degree.y);
  float M_PI = 3.14159265359;
  vec2 angles = 1.001 * angles_degree * M_PI / 180;
  float r = 5400*4/2/M_PI + model_pos.y;
  vec3 cartesian = vec3(
    r*sin(angles.y)*cos(angles.x),
    r*sin(angles.y)*sin(angles.x),
    r*cos(angles.y)
  );

  return cartesian;
}

vec3 CDLODTerrain_worldPos2(vec3 model_pos) {
  vec2 angles_degree = model_pos.xz * (vec2(360, 180) / vec2(5400*4, 2700*4));
  angles_degree = vec2(360-angles_degree.x, 180-angles_degree.y);
  float M_PI = 3.14159265359;
  vec2 angles = 1.001 * angles_degree * M_PI / 180;
  vec3 cartesian = vec3(
    sin(angles.y)*cos(angles.x),
    sin(angles.y)*sin(angles.x),
    cos(angles.y)
  );

  return cartesian;
}


vec4 CDLODTerrain_modelPos() {
  vec2 pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * CDLODTerrain_aPosition;

  float max_dist = 2.35*pow(2.5, CDLODTerrain_uLevel) * CDLODTerrain_uNodeDimension;
  vec3 estimated_pos1 = vec3(pos.x, 0, pos.y);
  vec3 estimated_pos2 = vec3(pos.x, 100, pos.y);
  float dist = min(
    length(CDLODTerrain_uCamPos - CDLODTerrain_worldPos(estimated_pos1)),
    length(CDLODTerrain_uCamPos - CDLODTerrain_worldPos(estimated_pos2))
  );

  float start_dist = max(2.15/2.35*max_dist, max_dist - sqrt(max_dist));
  float dist_from_start = dist - start_dist;
  float start_to_end_dist = max_dist - start_dist;
  float morph = dist_from_start / start_to_end_dist;
  morph = clamp(morph, 0.0, 1.0);

  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  morphed_pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * morphed_pos;
  float height = CDLODTerrain_fetchHeight(morphed_pos, morph);
  return vec4(morphed_pos.x, height, morphed_pos.y, morph);
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_uTexSize;
}

vec3 CDLODTerrain_normal(vec4 pos) {
  vec3 u = vec3(1.0f, CDLODTerrain_fetchHeight(pos.xz + vec2(1, 0), pos.w) -
                      CDLODTerrain_fetchHeight(pos.xz - vec2(1, 0), pos.w), 0.0f);
  vec3 v = vec3(0.0f, CDLODTerrain_fetchHeight(pos.xz + vec2(0, 1), pos.w) -
                      CDLODTerrain_fetchHeight(pos.xz - vec2(0, 1), pos.w), 1.0f);
  return normalize(cross(u, -v));
}
