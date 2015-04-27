// Copyright (c) 2015, Tamas Csala

#version 330

#include "engine/cdlod_terrain.vert"

uniform mat4 uProjectionMatrix, uCameraMatrix, uModelMatrix;
float CDLODTerrain_getHeight(vec2 sample, float morph);
int CDLODTerrain_uLevel;

out VertexData {
  out vec3  c_pos, w_pos, m_pos;
  out vec2  texCoord;
  out float level, morph, invalid;
} vOut;

void main() {
  vec4 temp = CDLODTerrain_modelPos();
  vec3 m_pos = temp.xyz;
  vOut.morph = temp.w;
  if (!CDLODTerrain_isVisible(m_pos)) {
    vOut.invalid = 1.0;
    return;
  } else {
    vOut.invalid = 0.0;
  }
  vOut.m_pos = m_pos;

  vec2 tex_coord = CDLODTerrain_texCoord(m_pos);

  vec3 w_pos = CDLODTerrain_worldPos(m_pos);
  vec3 offseted_w_pos = (uModelMatrix * vec4(w_pos, 1)).xyz;

  vOut.w_pos = offseted_w_pos;
  vOut.texCoord = tex_coord;

  vec4 c_pos = uCameraMatrix * vec4(offseted_w_pos, 1);
  vOut.c_pos = vec3(c_pos);

  vOut.level = CDLODTerrain_uLevel;
  gl_Position = uProjectionMatrix * c_pos;
}
