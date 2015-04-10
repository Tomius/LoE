// Copyright (c) 2015, Tamas Csala

#version 330

in vec4 aPosition;
in vec2 aTexCoord;

uniform mat4 uProjectionMatrix;
uniform vec2 uOffset;

out vec2 vTexCoord;

void main() {
  vTexCoord = aTexCoord;
  gl_Position = uProjectionMatrix*aPosition + vec4(uOffset, 0, 0);
}
