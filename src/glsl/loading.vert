// Copyright (c) 2015, Tamas Csala

#version 440

attribute vec4 aPosition;
attribute vec2 aTexCoord;

varying vec2 vTexCoord;

void main() {
  vTexCoord = vec2(aTexCoord.s, 1-aTexCoord.t);
  gl_Position = aPosition;
}
