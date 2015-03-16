// Copyright (c) 2015, Tamas Csala

#version 440

attribute vec2 aPosition, aTexCoord;

uniform vec2 uOffset, uScale;

out vec2 vTexCoord;

void main() {
  vTexCoord = aTexCoord;
  gl_Position = vec4(uScale * aPosition + uOffset, 0, 1);
}
