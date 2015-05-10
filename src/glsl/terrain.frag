// Copyright (c) 2015, Tamas Csala

#version 330
//#extension GL_ARB_texture_query_lod : enable

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
                    out ivec3[4] texel_coords, out vec4 weights) {
  vec2 sp = sample_coord.xy; // sample position in the texture
  int layer = int(sample_coord.z);

  // get the four nearest points
  ivec2 tl = ivec2(sp); // actually this one is the bottom left, but w/e
  ivec2 tr = tl + ivec2(1, 0);
  ivec2 bl = tl + ivec2(0, 1);
  ivec2 br = tl + ivec2(1, 1);

  // Note: the weight scales with 1-distance, so basically
  // the weight of top left, is the distance of bottom right
  weights.x = (br.x - sp.x)*(br.y - sp.y); //tl
  weights.y = (sp.x - bl.x)*(bl.y - sp.y); //tr
  weights.z = (tr.x - sp.x)*(sp.y - tr.y); //bl
  weights.w = (sp.x - tl.x)*(sp.y - tl.y); //br

  // clamp coordinates that hang out of the entire atlas
  // this must be done after calculating the weights
  if (layer % kAtlasSize.x == 0) {
    tr.x = max(tr.x, 0);
    br.x = max(br.x, 0);
  } else if (layer % kAtlasSize.x == kAtlasSize.x - 1) {
    tr.x = min(tr.x, tex_size.x - 1);
    br.x = min(br.x, tex_size.x - 1);
  }

  if (layer / kAtlasSize.x == 0) {
    bl.y = max(bl.y, 0);
    br.y = max(br.y, 0);
  } else if (layer / kAtlasSize.x == kAtlasSize.y - 1) {
    bl.y = min(bl.y, tex_size.y - 1);
    br.y = min(br.y, tex_size.y - 1);
  }

  // some texels might be in a different texture

  // the top left is guaranteed to not hang out of the texture
  texel_coords[0] = ivec3(tl, layer);

  // tr.x can be past the right edge of the altas elem
  if (tr.x < tex_size.x) {
    // if it is not on the edge
    texel_coords[1] = ivec3(tr, layer);
  } else {
    // if the texel hangs out to the right
    texel_coords[1] = ivec3(0, tr.y, layer+1);
  }

  // bl.y can be past the bottom edge of the altas elem
  if (bl.y < tex_size.y) {
    // if it is not on the edge
    texel_coords[2] = ivec3(bl, layer);
  } else {
    // if the texel hangs out to the bottom
    texel_coords[2] = ivec3(bl.x, 0, layer+kAtlasSize.x);
  }

  // br can be past the bottom and the right edge of the altas elem
  if (br.x < tex_size.x && br.y < tex_size.y) {
    // if it is not on the edge on either directions
    texel_coords[3] = ivec3(br, layer);
  } else if (br.x < tex_size.x) {
    // if the texel hangs out to the bottom
    texel_coords[3] = ivec3(br.x, 0, layer+kAtlasSize.x);
  } else if (br.y < tex_size.y) {
    // if the texel hangs out to the right
    texel_coords[3] = ivec3(0, br.y, layer+1);
  } else {
    // if the texel hangs out on both directions
    texel_coords[3] = ivec3(0, 0, layer+kAtlasSize.x+1);
  }
}

vec3 getSampleCoord(ivec2 tex_size) {
  vec2 scaled_tex_coord = vIn.m_pos.xz / CDLODTerrain_GeomSize * kAtlasSize;
  ivec2 atlas_coord = ivec2(scaled_tex_coord);
  ivec2 atlas_elem_size = CDLODTerrain_GeomSize / kAtlasSize;
  ivec2 atlas_elem_top = atlas_coord * atlas_elem_size;
  vec2 relative_coord = vIn.m_pos.xz - atlas_elem_top;
  return vec3(relative_coord * (vec2(tex_size) / atlas_elem_size),
              kAtlasSize.x * atlas_coord.y + atlas_coord.x);
}

// vec3 transformTexcoord(vec2 tex_coord) {
//   vec2 scaled_tex_coord = tex_coord * kAtlasSize;
//   ivec2 atlas_coord = ivec2(scaled_tex_coord);
//   return vec3(scaled_tex_coord - atlas_coord,
//               kAtlasSize.x * atlas_coord.y + atlas_coord.x);
// }

vec3 getColor() {
  // vec3 tex_coord = transformTexcoord(vIn.texCoord);
  // float mipmapLevel = textureQueryLOD(uDiffuseTexture, tex_coord.xy).x;
  // int level = int(round(mipmapLevel));
  int level = 0;
  ivec2 tex_size = textureSize(uDiffuseTexture, level).xy;
  vec3 sample_coord = getSampleCoord(tex_size);
  ivec3[4] texel_coords; vec4 weights;
  bilinearSample(sample_coord, tex_size, texel_coords, weights);

  vec3 color = vec3(0);
  for (int i = 0; i < 4; ++i) {
    color += texelFetch(uDiffuseTexture, texel_coords[i], level).xyz * weights[i];
  }

  return color;
}

void main() {
  // Normals
  vec3 w_normal = normalize(vIn.w_normal);

  // Lighting
  vec3 lighting = HemisphereLighting(w_normal);
  vec3 w_sun_dir = SunPos();
  float diffuse_power = CalculateLighting(w_normal, w_sun_dir);
  diffuse_power *= pow(SunPower(), 0.3);
  lighting += SunColor() * diffuse_power;

  //vec3 diffuse_color = texture(uDiffuseTexture, transformTexcoord(vIn.texCoord)).rgb;
  vec3 diffuse_color = getColor();

  vec3 final_color = sqrt(diffuse_color) * (AmbientPower() + lighting);

  fragColor = vec4(final_color, 1);
  //fragColor = vec4(vIn.level/8, vIn.morph/3, 0, 1)*0.5 + 0.5*vec4(final_color, 1);
}
