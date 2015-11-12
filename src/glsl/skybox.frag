// Copyright (c) 2015, Tamas Csala

#version 330

#include "sky.frag"

in vec3 vTexCoord;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out float fragDepth;

void main() {
  fragColor = vec4(SkyColor(normalize(vTexCoord)), 1.0);
  fragDepth = 0.0;
}
