#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in VertexData {
  vec3 c_pos, w_pos, m_pos;
  vec2 texCoord;
  float level, morph, invalid;
} vIn[];

out VertexData {
  vec3 w_normal;
  vec3 c_pos, w_pos, m_pos;
  vec2 texCoord;
  float level, morph;
} vOut;

void main() {
  for (int i = 0; i < gl_in.length(); i++) {
    if (vIn[i].invalid == 1.0) {
      return;
    }
  }

  vec3 a = vIn[0].w_pos, b = vIn[1].w_pos, c = vIn[2].w_pos;
  vec3 normal = cross(b-a, c-a);


  for (int i = 0; i < gl_in.length(); i++) {
    vOut.w_normal = normal;
    vOut.c_pos = vIn[i].c_pos;
    vOut.w_pos = vIn[i].w_pos;
    vOut.m_pos = vIn[i].m_pos;
    vOut.texCoord = vIn[i].texCoord;
    vOut.level = vIn[i].level;
    vOut.morph = vIn[i].morph;
    gl_Position = gl_in[i].gl_Position;

    EmitVertex();
  }
  EndPrimitive();
}
