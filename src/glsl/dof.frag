// Copyright (c) 2015, Tamas Csala

#version 330

#export float DistanceFromCamera();
#export vec3 DoF(vec3 texel_color);
#export uniform float uZFar;

uniform sampler2D uTex, uDepthTex;
uniform float uDepthCoef;
uniform vec2 uResolution;

vec2 coord = ivec2(gl_FragCoord.xy) / uResolution;
int mipmap_count = 1 + int(log2(max(uResolution.x, uResolution.y)));

float DistanceFromCamera_Internal() {
  return texture(uDepthTex, coord).x;
}

float dist_from_cam = DistanceFromCamera_Internal();

float DistanceFromCamera() {
  return dist_from_cam;
}

vec3 DoF(vec3 texel_color) {
  float level = DistanceFromCamera() / uZFar * (mipmap_count-1);
  float floor_level = clamp(floor(level), 0, mipmap_count-1);
  vec3 color = 1.5*texel_color;
  for (int i = 1; i <= floor_level; ++i) {
    color += textureLod(uTex, coord, float(i)).rgb;
  }
  color += (level-floor_level) * textureLod(uTex, coord, floor_level+1).rgb;
  return color / (1.5 + level);
}
