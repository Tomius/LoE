// Copyright (c) 2014, Tamas Csala

#version 440

#export vec4 CDLODTerrain_worldPos();
#export vec2 CDLODTerrain_texCoord(vec3 pos);
#export vec3 CDLODTerrain_normal(vec3 pos);
#export mat3 CDLODTerrain_normalMatrix(vec3 normal);

in vec2 CDLODTerrain_aPosition; // I hate the lack of namespaces

// Vertex attrib divisor works like a uniform
in vec4 CDLODTerrain_uRenderData;

vec2 CDLODTerrain_uOffset = CDLODTerrain_uRenderData.xy;
float CDLODTerrain_uScale = CDLODTerrain_uRenderData.z;
int CDLODTerrain_uLevel = int(CDLODTerrain_uRenderData.w);

uniform sampler2D CDLODTerrain_uHeightMap;
uniform vec2 CDLODTerrain_uTexSize;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;

float CDLODTerrain_fetchHeight(vec2 tex_coord) {
#if 0
  return texelFetch(CDLODTerrain_uHeightMap,
                    ivec2(tex_coord) / (1 << CDLODTerrain_uLevel),
                    CDLODTerrain_uLevel).r * 255;
#else
  return texture(CDLODTerrain_uHeightMap,
                 tex_coord / vec2(CDLODTerrain_uTexSize)).r * 255;
#endif
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5 ) * 2.0;
  return (vertex - frac_part * morph);
}

vec4 CDLODTerrain_worldPos() {
  vec2 pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * CDLODTerrain_aPosition;

  float max_dist = 2* CDLODTerrain_uScale * CDLODTerrain_uNodeDimension;
  float dist = length(CDLODTerrain_uCamPos - vec3(pos.x, CDLODTerrain_fetchHeight(pos), pos.y));

  float start_dist = 0.95*max_dist - sqrt(0.95*max_dist);
  float dist_from_start = dist - start_dist;
  float start_to_end_dist = max_dist - start_dist;
  float morph = dist_from_start / start_to_end_dist;
  morph = clamp(morph, 0.0, 1.0);


  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  morphed_pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * morphed_pos;
  return vec4(morphed_pos.x, CDLODTerrain_fetchHeight(morphed_pos), morphed_pos.y, morph);
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_uTexSize;
}

vec3 CDLODTerrain_normal(vec3 pos) {
  vec3 u = vec3(1.0f, CDLODTerrain_fetchHeight(pos.xz + vec2(1, 0)) -
                      CDLODTerrain_fetchHeight(pos.xz - vec2(1, 0)), 0.0f);
  vec3 v = vec3(0.0f, CDLODTerrain_fetchHeight(pos.xz + vec2(0, 1)) -
                      CDLODTerrain_fetchHeight(pos.xz - vec2(0, 1)), 1.0f);
  return normalize(cross(u, -v));
}

mat3 CDLODTerrain_normalMatrix(vec3 normal) {
  vec3 tangent = normalize(cross(normal, vec3(0.0, 0.0, 1.0)));
  vec3 bitangent = normalize(cross(normal, tangent));
  tangent = normalize(cross(bitangent, normal));

  return mat3(tangent, bitangent, normal);
}
