// Copyright (c) 2015, Tamas Csala

#version 330
#extension GL_ARB_texture_query_lod : enable

#include "sky.frag"
#include "engine/cdlod_terrain.frag"

in VertexData {
  vec3 w_normal;
  vec3 c_pos, w_pos, m_pos;
  vec2 texCoord;
  float level, morph;
  vec4 render_data;
} vIn;

uniform mat4 uCameraMatrix;
uniform sampler2DArray uDiffuseTexture;

uniform float CDLODTerrain_uNodeDimension;
uniform ivec2 CDLODTerrain_uTexSize;
uniform vec3 CDLODTerrain_uCamPos;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out float fragDepth;

const ivec2 kAtlasSize = ivec2(4, 4);
const float kShininess = 4;

// Quite a weird way to "simulate" amibent occlusion, but works suprisingly good
float CalculateLighting(vec3 normal, vec3 light_dir) {
  float d = dot(normal, light_dir);

  vec3 view_dir = normalize(CDLODTerrain_uCamPos - vIn.w_pos);
  vec3 half_vec = normalize(light_dir + view_dir);
  float d2 = dot(half_vec, normal);
  float specular_power = pow(max(d2, 0), kShininess) + pow(0.25*(d2+1), kShininess);

  return max(d, 0) + 0.05*(d+1) /*+ 0.2*specular_power*/;
}

void handleBorders(ivec2 tex_size, inout ivec3[4] tc,
                   out bool is_on_layer_border) {
  // some texels might be in a different texture
  // at first, assume that the sampling is not special (is not on layer border)
  is_on_layer_border = false;

  for(int i = 0; i < 4; ++i) {
    if (tc[i].x < 0) {
      // if the texel hangs out to the left
      if (tc[i].z % kAtlasSize.x == 0) {
        tc[i] = ivec3(tex_size.x-1, tc[i].y, tc[i].z+kAtlasSize.x-1);
      } else {
        tc[i] = ivec3(tex_size.x-1, tc[i].y, tc[i].z-1);
      }
      is_on_layer_border = true;
    } else if (tex_size.x <= tc[i].x) {
      // if the texel hangs out to the right
      if (tc[i].z % kAtlasSize.x == kAtlasSize.x - 1) {
        tc[i] = ivec3(0, tc[i].y, tc[i].z-kAtlasSize.x+1);
      } else {
        tc[i] = ivec3(0, tc[i].y, tc[i].z+1);
      }
      is_on_layer_border = true;
    }

    if (tc[i].y < 0) {
      // if the texel hangs out to the top
      tc[i] = ivec3(tc[i].x, tex_size.y-1, tc[i].z-kAtlasSize.x);
      is_on_layer_border = true;
    } else if (tex_size.y <= tc[i].y) {
      // if the texel hangs out to the bottom
      tc[i] = ivec3(tc[i].x, 0, tc[i].z+kAtlasSize.x);
      is_on_layer_border = true;
    }
  }
}

void bilinearSample(vec3 sample_coord, ivec2 tex_size,
                    out ivec3[4] texel_coords, out vec4 weights,
                    out bool is_on_layer_border) {
  vec2 sp = sample_coord.xy; // sample position in the texture
  // A half pixel offset is needed to make the calculations correct,
  // and get the same result as the with the built-in texture functions.
  // (though it makes the calculation a bit more complicated)
  sp -= vec2(0.5);
  int layer = int(sample_coord.z);

  // get the four nearest points
  ivec2 tl = ivec2(floor(sp)); // actually this one is the bottom left, but w/e
  ivec2 tr = tl + ivec2(1, 0);
  ivec2 bl = tl + ivec2(0, 1);
  ivec2 br = tl + ivec2(1, 1);

  // Note: the weight scales with 1-distance, so basically
  // the weight of top left, is the distance of bottom right
  weights.x = (br.x - sp.x)*(br.y - sp.y); //tl
  weights.y = (sp.x - bl.x)*(bl.y - sp.y); //tr
  weights.z = (tr.x - sp.x)*(sp.y - tr.y); //bl
  weights.w = (sp.x - tl.x)*(sp.y - tl.y); //br

  texel_coords[0] = ivec3(tl, layer);
  texel_coords[1] = ivec3(tr, layer);
  texel_coords[2] = ivec3(bl, layer);
  texel_coords[3] = ivec3(br, layer);

  handleBorders(tex_size, texel_coords, is_on_layer_border);
}

vec3 getSampleCoord(ivec2 tex_size) {
  vec2 pos = vIn.m_pos.xz;
  vec2 max_tex_coord = tex_size * kAtlasSize;
  vec2 global_texel_coord = pos * (max_tex_coord / CDLODTerrain_uTexSize);

  ivec2 atlas_coord = ivec2(global_texel_coord / tex_size);
  ivec2 global_atlas_elem_texel_top = atlas_coord * tex_size;
  vec2 relative_coord = global_texel_coord - global_atlas_elem_texel_top;
  return vec3(relative_coord, kAtlasSize.x * atlas_coord.y + atlas_coord.x);
}

vec3 getColor(int level) {
  ivec2 tex_size = textureSize(uDiffuseTexture, level).xy;
  vec3 sample_coord = getSampleCoord(tex_size);

  // manual sampling is only needed in case of magnification.
  if (level != 0) {
    vec3 tex_coord = sample_coord / ivec3(tex_size, 1);
    return textureLod(uDiffuseTexture, tex_coord, level).xyz;
  }

  ivec3[4] texel_coords; vec4 weights; bool is_on_layer_border;
  bilinearSample(sample_coord, tex_size, texel_coords, weights, is_on_layer_border);

  if (is_on_layer_border) {
    vec3 color = vec3(0);
    for (int i = 0; i < 4; ++i) {
      color += texelFetch(uDiffuseTexture, texel_coords[i], level).xyz * weights[i];
    }

    return color;
  } else {
    vec3 tex_coord = sample_coord / ivec3(tex_size, 1);
    return textureLod(uDiffuseTexture, tex_coord, level).xyz;
  }
}

vec3 getColor() {
  float mipmapLevel = textureQueryLOD(uDiffuseTexture, vIn.texCoord * kAtlasSize).x;
  int level0 = int(floor(mipmapLevel)), level1 = int(ceil(mipmapLevel));
  vec3 color0 = getColor(level0), color1 = getColor(level1);
  return (level0+1-mipmapLevel)*color0 + (mipmapLevel-level0)*color1;
}

void main() {
  // Lighting
  vec3 w_sun_pos = SunPos();
  float diffuse_power = 0.0;
  vec3 m_normal;
  CDLODTerrain_modelPos(vIn.m_pos.xz, vIn.render_data, m_normal);
  vec3 m_normal_offseted_pos = vIn.m_pos + m_normal;
  vec3 w_normal = CDLODTerrain_worldPos(m_normal_offseted_pos)
                  - CDLODTerrain_worldPos(vIn.m_pos);
  w_normal = normalize(w_normal);

  vec3 w_sun_dir = normalize(w_sun_pos);
  diffuse_power = CalculateLighting(w_normal, w_sun_dir);

  vec3 lighting = vec3(diffuse_power);

  float gamma = 2.2;
  vec3 diffuse_color = pow(getColor(), vec3(1/gamma));
  vec3 final_color = diffuse_color * (0.05 + lighting);

  fragColor = vec4(final_color, 1);
  fragDepth = length(vIn.c_pos);
  // fragColor = vec4(vIn.level/8, vIn.morph/3, 0, 1)*0.25 + 0.75*vec4(final_color, 1);
}
