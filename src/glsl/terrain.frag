// Copyright (c) 2015, Tamas Csala

#version 330

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
uniform sampler2D uDiffuseTexture;

out vec4 fragColor;

const float kSpecularShininess = 64.0;

float CalculateLighting(vec3 normal, vec3 light_dir) {
  return max(dot(normal, light_dir), 0.0);
}

uniform float CDLODTerrain_uNodeDimension;

void main() {
  // Normals
  vec3 w_normal = normalize(vIn.w_normal);

  // Lighting
  vec3 lighting = HemisphereLighting(w_normal);
  vec3 w_sun_dir = SunPos();
  float diffuse_power = CalculateLighting(w_normal, w_sun_dir);
  diffuse_power *= pow(SunPower(), 0.3);
  diffuse_power = 0.5*diffuse_power + 0.5; // to avoid shading artifacts
  lighting += SunColor() * diffuse_power;

  vec3 diffuse_color = sqrt(texture2D(uDiffuseTexture, vIn.texCoord).rgb);

  vec3 final_color = diffuse_color * (AmbientPower() + lighting);

  fragColor = vec4(final_color, 1);
  //fragColor = vec4(vIn.level/8, vIn.morph, 0, 1)*0.5 + 0.5*vec4(final_color, 1);
}
