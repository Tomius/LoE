// Copyright (c) 2014, Tamas Csala

#version 440

#include "sky.frag"
#include "fog.frag"
#include "hemisphere_lighting.frag"

in vec3  w_vNormal;
in vec3  c_vPos, w_vPos, m_vPos;
in vec2  vTexCoord;
in float vLevel, vMorph;

uniform mat4 uCameraMatrix;
uniform sampler2D uDiffuseTexture;

const float kSpecularShininess = 64.0;

void CalculateLighting(vec3 c_normal, vec3 c_light_dir,
                       out float diffuse_power, out float specular_power) {
  vec3 c_view_dir = -normalize((uCameraMatrix * vec4(w_vPos, 1)).xyz);

  diffuse_power = dot(c_normal, c_light_dir);
  if (diffuse_power <= 0.0) {
    diffuse_power = 0.0;
    specular_power = 0.0;
  } else {
    vec3 L = c_light_dir, V = c_view_dir;
    vec3 H = normalize(L + V), N = c_normal;
    specular_power = pow(max(dot(H, N), 0), kSpecularShininess);
  }
}

void main() {
  // Normals
  vec3 w_normal = normalize(w_vNormal);
  vec3 c_normal = mat3(uCameraMatrix) * w_normal;

  // Lighting
  vec3 lighting = HemisphereLighting(w_normal);
  vec3 w_sun_dir = SunPos();
  float diffuse_power, specular_power;
  vec3 c_sun_dir = mat3(uCameraMatrix) * w_sun_dir;
  CalculateLighting(c_normal, c_sun_dir, diffuse_power, specular_power);
  diffuse_power *= pow(SunPower(), 0.3);
  specular_power *= pow(SunPower(), 0.3);
  lighting += SunColor() * (diffuse_power + specular_power);

  vec3 diffuse_color = texture2D(uDiffuseTexture, vTexCoord).rgb;

  vec3 final_color = diffuse_color * (AmbientPower() + lighting);

  gl_FragColor = vec4(final_color, 1);
  //gl_FragColor = vec4(vLevel/8, vMorph, 0, 1)*0.99 + 0.01*vec4(final_color, 1);
}
