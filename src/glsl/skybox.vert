// Copyright (c) 2015, Tamas Csala

#version 330

in vec3 aPosition;

uniform mat4 uProjectionMatrix;
uniform mat3 uCameraMatrix;
uniform float uScale;

out vec3 vTexCoord;

void main(void) {
  gl_Position = uProjectionMatrix * vec4(uCameraMatrix * vec3(uScale * aPosition), 1.0);
  vTexCoord = aPosition;
}
