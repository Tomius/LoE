// Copyright (c) 2015, Tamas Csala

#version 440

in vec2 vTexCoord;
uniform sampler2D uTex;
uniform vec4 uColor;

void main() {
  gl_FragColor = vec4(uColor.rgb, uColor.a * texture2D(uTex, vTexCoord).r);
}
