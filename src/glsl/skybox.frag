// Copyright (c) 2015, Tamas Csala

#version 440

#include "sky.frag"

in vec3 vTexCoord;

void main() {
  gl_FragColor = vec4(SkyColor(normalize(vTexCoord)), 1.0);
}
