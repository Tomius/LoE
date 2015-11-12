// Copyright (c) 2015, Tamas Csala

#version 330

in vec3 aPosition;

uniform mat4 uProjectionMatrix;
uniform mat3 uCameraMatrix;

out vec3 vTexCoord;

void main(void) {
  gl_Position = uProjectionMatrix * vec4(uCameraMatrix * aPosition, 1.0);
  gl_Position.z = 0.5*gl_Position.w;
  vTexCoord = aPosition;
}
