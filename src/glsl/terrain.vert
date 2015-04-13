// Copyright (c) 2015, Tamas Csala

#version 330

#include "engine/cdlod_terrain.vert"

uniform mat4 uProjectionMatrix, uCameraMatrix, uModelMatrix;
uniform ivec2 CDLODTerrain_uTexSize;
float CDLODTerrain_getHeight(vec2 sample, float morph);

out vec3  w_vNormal;
out vec3  c_vPos, w_vPos, m_vPos;
out vec2  vTexCoord;
out float vLevel, vMorph;

int CDLODTerrain_uLevel;

void main() {
  vec4 temp = CDLODTerrain_modelPos();
  vec3 m_pos = temp.xyz;
  vMorph = temp.w;
  if (m_pos.x < -1 || CDLODTerrain_uTexSize.x + 1 < m_pos.x ||
      m_pos.z < -1 || CDLODTerrain_uTexSize.y + 1 < m_pos.z) {
    m_pos.xz = clamp(m_pos.xz, vec2(0), CDLODTerrain_uTexSize);
    m_pos.y = CDLODTerrain_getHeight(m_pos.xz, 0.0);
  }
  m_vPos = m_pos;

  vec2 tex_coord = CDLODTerrain_texCoord(m_pos);

  vec3 w_pos = CDLODTerrain_worldPos(m_pos);
  vec3 offseted_w_pos = (uModelMatrix * vec4(w_pos, 1)).xyz;

  w_vPos = offseted_w_pos;
  vTexCoord = tex_coord;

  vec4 c_pos = uCameraMatrix * vec4(offseted_w_pos, 1);
  c_vPos = vec3(c_pos);

  w_vNormal = CDLODTerrain_normal(vec4(m_pos, temp.w));
  vLevel = CDLODTerrain_uLevel;
  gl_Position = uProjectionMatrix * c_pos;
}
