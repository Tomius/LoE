// Copyright (c) 2015, Tamas Csala

#version 330

#include "sky.frag"
#include "fog.frag"
#include "hemisphere_lighting.frag"

in vec3  w_vNormal;
in vec3  c_vPos, w_vPos, m_vPos;
in vec2  vTexCoord;
in float vLevel, vMorph;

uniform mat4 uCameraMatrix;
uniform sampler2D uDiffuseTexture;

out vec4 fragColor;

const float kSpecularShininess = 64.0;

float CalculateLighting(vec3 c_normal, vec3 c_light_dir) {
  vec3 c_view_dir = -normalize((uCameraMatrix * vec4(w_vPos, 1)).xyz);
  return max(dot(c_normal, c_light_dir), 0.0);
}

void main() {
  // Normals
  vec3 w_normal = normalize(w_vNormal);
  vec3 c_normal = mat3(uCameraMatrix) * w_normal;

  // Lighting
  vec3 lighting = HemisphereLighting(w_normal);
  vec3 w_sun_dir = SunPos();
  vec3 c_sun_dir = mat3(uCameraMatrix) * w_sun_dir;
  float diffuse_power = CalculateLighting(c_normal, c_sun_dir);
  diffuse_power *= pow(SunPower(), 0.3);
  lighting += SunColor() * diffuse_power;

  vec3 diffuse_color = texture2D(uDiffuseTexture, vTexCoord).rgb;

  vec3 final_color = diffuse_color * (AmbientPower() + lighting);

  fragColor = vec4(final_color, 1);
  //fragColor = vec4(vLevel/8, vMorph, 0, 1)*0.99 + 0.01*vec4(final_color, 1);
}
