// Copyright (c) 2015, Tamas Csala

#version 130

in vec2 vTexCoord;
uniform sampler2D uTex;
uniform vec4 uColor;

out vec4 fragColor;

void main() {
  fragColor = vec4(uColor.rgb, uColor.a * texture2D(uTex, vTexCoord).r);
}
