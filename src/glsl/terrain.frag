// Copyright (c) 2014, Tamas Csala

#version 440

#include "sky.frag"
#include "fog.frag"
#include "hemisphere_lighting.frag"

in vec3  w_vNormal;
in vec3  c_vPos, w_vPos, m_vPos;
in vec2  vTexCoord;
in mat3  vNormalMatrix;
in float vLevel, vMorph;

uniform mat4 uCameraMatrix;
uniform sampler2D uGrassMap0, uGrassMap1, uGrassNormalMap;

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
  if (m_vPos.y == 0) {
    gl_FragColor = vec4(0, 0, 1, 1);
    return;
  }

  // Normals
  mat3 normal_matrix;
  normal_matrix[0] = normalize(vNormalMatrix[0]);
  normal_matrix[1] = normalize(vNormalMatrix[1]);
  normal_matrix[2] = normalize(vNormalMatrix[2]);
  vec3 normal_offset = texture2D(uGrassNormalMap, vTexCoord*256).rgb;
  vec3 w_normal = normalize(normal_matrix[2] + normal_matrix * normal_offset);
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

  vec3 grass_color_0 = texture2D(uGrassMap0, vTexCoord*256).rgb;
  vec3 grass_color_1 = texture2D(uGrassMap0, vTexCoord*16).rgb;

  vec3 rock_color_0 = texture2D(uGrassMap1, vTexCoord*256).rgb;
  vec3 rock_color_1 = texture2D(uGrassMap1, vTexCoord*16).rgb;

  float height_factor = clamp(sqrt(max(m_vPos.y - 80, 0) / 128), 0, 1);

  vec3 color_0 = mix(grass_color_0, rock_color_0, height_factor);
  vec3 color_1 = mix(grass_color_1, rock_color_1, height_factor/2);
  vec3 diffuse_color = mix(color_0, color_1, 0.5);

  vec3 final_color = diffuse_color * (AmbientPower() + lighting);

  //gl_FragColor = vec4(final_color, 1);
  gl_FragColor = /*vec4(vLevel/8, vMorph, 0, 1)*0.99 + 0.01**/vec4(final_color, 1);
}
