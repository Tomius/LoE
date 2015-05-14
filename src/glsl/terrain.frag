// Copyright (c) 2015, Tamas Csala

#version 330
#extension GL_ARB_texture_query_lod : enable

#include "sky.frag"
#include "fog.frag"
#include "hemisphere_lighting.frag"

in VertexData {
  vec3 w_normal;
  vec3 c_pos, w_pos, m_pos;
  vec2 texCoord;
  float level, morph;
} vIn;

uniform mat4 uCameraMatrix;
uniform sampler2DArray uDiffuseTexture;

out vec4 fragColor;

const float kSpecularShininess = 64.0;
const ivec2 kAtlasSize = ivec2(4, 4);

float CalculateLighting(vec3 normal, vec3 light_dir) {
  return max(dot(normal, light_dir), 0.0);
}

uniform float CDLODTerrain_uNodeDimension;

uniform ivec2 CDLODTerrain_uTexSize;
uniform int CDLODTerrain_uGeomDiv;
ivec2 CDLODTerrain_GeomSize = CDLODTerrain_uTexSize << CDLODTerrain_uGeomDiv;

// bilinear sampling
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

  // some texels might be in a different texture
  // at first, assume that the sampling is not special (is not on layer border)
  is_on_layer_border = false;

  // the top left can hang out on the top and left edges
  if (0 <= tl.x && 0.0 <= tl.y) {
    // if it is not on the edge on either directions
    texel_coords[0] = ivec3(tl, layer);
  } else if (0 <= tl.x) {
    // if the texel hangs out to the top
    texel_coords[0] = ivec3(tl.x, tex_size.y-1, layer-kAtlasSize.x);
    is_on_layer_border = true;
  } else if (0.0 <= tl.y) {
    // if the texel hangs out to the left
    texel_coords[0] = ivec3(tex_size.x-1, tl.y, layer-1);
    is_on_layer_border = true;
  } else {
    // if the texel hangs out on both directions
    texel_coords[0] = ivec3(tex_size.x-1, tex_size.y-1, layer-kAtlasSize.x-1);
    is_on_layer_border = true;
  }

  // the top right can hang out on the top and right edges
  if (tr.x < tex_size.x && 0.0 <= tr.y) {
    // if it is not on the edge on either directions
    texel_coords[1] = ivec3(tr, layer);
  } else if (tr.x < tex_size.x) {
    // if the texel hangs out to the top
    texel_coords[1] = ivec3(tr.x, tex_size.y-1, layer-kAtlasSize.x);
    is_on_layer_border = true;
  } else if (0.0 <= tr.y) {
    // if the texel hangs out to the right
    texel_coords[1] = ivec3(0, tr.y, layer+1);
    is_on_layer_border = true;
  } else {
    // if the texel hangs out on both directions
    texel_coords[1] = ivec3(0, tex_size.y-1, layer-kAtlasSize.x+1);
    is_on_layer_border = true;
  }

  // the bottom left can hang out on the bottom and left edges
  if (0 <= bl.x && bl.y < tex_size.y) {
    // if it is not on the edge on either directions
    texel_coords[2] = ivec3(bl, layer);
  } else if (0 <= bl.x) {
    // if the texel hangs out to the bottom
    texel_coords[2] = ivec3(bl.x, 0, layer+kAtlasSize.x);
    is_on_layer_border = true;
  } else if (bl.y < tex_size.y) {
    // if the texel hangs out to the left
    texel_coords[2] = ivec3(tex_size.x-1, bl.y, layer-1);
    is_on_layer_border = true;
  } else {
    // if the texel hangs out on both directions
    texel_coords[2] = ivec3(tex_size.x-1, 0, layer+kAtlasSize.x-1);
    is_on_layer_border = true;
  }

  // br can be past the bottom and the right edge of the altas elem
  if (br.x < tex_size.x && br.y < tex_size.y) {
    // if it is not on the edge on either directions
    texel_coords[3] = ivec3(br, layer);
  } else if (br.x < tex_size.x) {
    // if the texel hangs out to the bottom
    texel_coords[3] = ivec3(br.x, 0, layer+kAtlasSize.x);
    is_on_layer_border = true;
  } else if (br.y < tex_size.y) {
    // if the texel hangs out to the right
    texel_coords[3] = ivec3(0, br.y, layer+1);
    is_on_layer_border = true;
  } else {
    // if the texel hangs out on both directions
    texel_coords[3] = ivec3(0, 0, layer+kAtlasSize.x+1);
    is_on_layer_border = true;
  }
}

vec3 getSampleCoord(ivec2 tex_size) {
  vec2 pos = vIn.m_pos.xz;
  vec2 max_tex_coord = tex_size * kAtlasSize;
  vec2 global_texel_coord = pos * (max_tex_coord / CDLODTerrain_GeomSize);

  // Sampling at the absoulte borders is a bit buggy
  int global_edge_bias = 2;
  float scale = ((max_tex_coord-2*global_edge_bias)/max_tex_coord);
  global_texel_coord = global_texel_coord * scale + global_edge_bias;

  ivec2 atlas_coord = ivec2(global_texel_coord/tex_size);
  ivec2 global_atlas_elem_texel_top = atlas_coord * tex_size;
  vec2 relative_coord = global_texel_coord - global_atlas_elem_texel_top;
  return vec3(relative_coord, kAtlasSize.x * atlas_coord.y + atlas_coord.x);
}

vec3 getColor() {
  float mipmapLevel = textureQueryLOD(uDiffuseTexture, vIn.texCoord * kAtlasSize).x;
  int level = int(round(mipmapLevel));
  ivec2 tex_size = textureSize(uDiffuseTexture, level).xy;
  vec3 sample_coord = getSampleCoord(tex_size);
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

void main() {
  // Normals
  vec3 w_normal = normalize(vIn.w_normal);

  // Lighting
  vec3 w_sun_pos = SunPos();
  float diffuse_power = 0.0;
  if (dot(vIn.w_pos, w_sun_pos) > 0) {
    vec3 w_sun_dir = normalize(w_sun_pos);
    diffuse_power = CalculateLighting(w_normal, w_sun_dir);
  }

  vec3 lighting = vec3(diffuse_power);

  float gamma = 2.2;
  vec3 diffuse_color = pow(getColor(), vec3(1/gamma));
  vec3 final_color = diffuse_color * (0.2 + lighting);

  fragColor = vec4(final_color, 1);
  //fragColor = vec4(vIn.level/8, vIn.morph/3, 0, 1)*0.5 + 0.5*vec4(final_color, 1);
}
