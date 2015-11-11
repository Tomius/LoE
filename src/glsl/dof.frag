// Copyright (c) 2015, Tamas Csala

#version 330

#export float DistanceFromCamera();
#export vec3 DoF(vec3 texel_color);
#export uniform float uZFar;

uniform sampler2D uTex, uDepthTex;
uniform float uZNear, uZFar;
uniform vec2 uResolution;

vec2 coord = ivec2(gl_FragCoord.xy) / uResolution;
int mipmap_count = 1 + int(log2(max(uResolution.x, uResolution.y)));

// see http://web.archive.org/web/20130416194336/http://olivers.posterous.com/linear-depth-in-glsl-for-real
float DistanceFromCamera_Internal() {
  float z_b = texture(uDepthTex, coord).x;
  float z_n = 2.0 * z_b - 1.0;
  return 2.0 * uZNear * uZFar / (uZFar + uZNear - z_n * (uZFar - uZNear));
}

float dist_from_cam = DistanceFromCamera_Internal();

float DistanceFromCamera() {
  return dist_from_cam;
}

vec3 DoF(vec3 texel_color) {
  float level = sqrt(DistanceFromCamera() / uZFar) * 0.5 * (mipmap_count-1);
  float floor_level = floor(level);
  vec3 color = 2*texel_color;
  for (int i = 1; i <= floor_level; ++i) {
    color += textureLod(uTex, coord, float(i)).rgb;
  }
  color += (level-floor_level) * textureLod(uTex, coord, floor_level+1).rgb;
  return color / (2 + level);
}
