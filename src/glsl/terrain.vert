// Copyright (c) 2015, Tamas Csala

#version 330

#include "engine/cdlod_terrain.vert"

uniform mat4 uProjectionMatrix, uCameraMatrix, uModelMatrix;
int CDLODTerrain_uLevel;

out VertexData {
  vec3  w_normal;
  vec3  c_pos, w_pos, m_pos;
  vec2  texCoord;
  float level, morph;
} vOut;

void main() {
  vec3 m_normal;
  vec4 temp = CDLODTerrain_modelPos(m_normal);
  vec3 m_pos = temp.xyz;
  vOut.morph = temp.w;
  vOut.m_pos = m_pos;

  vec3 w_pos = CDLODTerrain_worldPos(m_pos);
  vec3 offseted_w_pos = (uModelMatrix * vec4(w_pos, 1)).xyz;
  vOut.w_pos = offseted_w_pos;

  vec3 m_normal_offseted_pos = m_pos + m_normal;
  vOut.w_normal = CDLODTerrain_worldPos(m_normal_offseted_pos) - w_pos;

  vOut.texCoord = CDLODTerrain_texCoord(m_pos);

  vec4 c_pos = uCameraMatrix * vec4(offseted_w_pos, 1);
  vOut.c_pos = vec3(c_pos);

  vOut.level = CDLODTerrain_uLevel;
  gl_Position = uProjectionMatrix * c_pos;
}
