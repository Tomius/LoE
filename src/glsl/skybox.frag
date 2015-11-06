// Copyright (c) 2015, Tamas Csala

#version 330

#include "sky.frag"
#include "scattering.frag"

in vec3 vTexCoord;

out vec4 fragColor;

void main() {
  vec3 scattering = Scattering();
  vec3 skycolor = SkyColor(normalize(vTexCoord));
  vec3 sumColor = skycolor + scattering;
  float luminance = 0.2126f*sumColor.r + 0.7152f*sumColor.g + 0.0722f*sumColor.b;
  float x = max(0, luminance-0.004);
  float newLuminance = (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06);

  fragColor = vec4(sumColor * (newLuminance / luminance), 1.0);
}
