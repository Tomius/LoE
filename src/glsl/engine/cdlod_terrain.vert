#version 330

#export vec4 CDLODTerrain_modelPos();
#export vec3 CDLODTerrain_worldPos(vec3 model_pos);
#export vec2 CDLODTerrain_texCoord(vec3 pos);
#export float CDLODTerrain_getHeight(vec2 sample, float morph);

in vec2 CDLODTerrain_aPosition; // I hate the lack of namespaces

// Vertex attrib divisor works like a uniform
in vec4 CDLODTerrain_uRenderData;

vec2 CDLODTerrain_uOffset = CDLODTerrain_uRenderData.xy;
float CDLODTerrain_uScale = CDLODTerrain_uRenderData.z;
int CDLODTerrain_uLevel = int(CDLODTerrain_uRenderData.w);

uniform ivec2 CDLODTerrain_uTexSize;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;

uniform int CDLODTerrain_max_level;
uniform usamplerBuffer CDLODTerrain_uHeightMap;
uniform usamplerBuffer CDLODTerrain_uHeightMapIndex;

struct CDLODTerrain_Node {
  ivec2 center, size;
  int level, index;
};

float M_PI = 3.14159265359;
float CDLODTerrain_cam_height =
    length(CDLODTerrain_uCamPos) - CDLODTerrain_uTexSize.x/2/M_PI;

CDLODTerrain_Node CDLODTerrain_getChildOf(CDLODTerrain_Node node,
                                          ivec2 tex_coord) {
  CDLODTerrain_Node child;
  if (tex_coord.y < node.center.y) {
    if (tex_coord.x < node.center.x) {
      // top left
      child.size = ivec2(node.size.x/2, node.size.y/2);
      child.center = node.center + ivec2(-child.size.x + child.size.x/2,
                                         -child.size.y + child.size.y/2);
      child.index = 4*node.index + 1;
    } else {
      // top right
      child.size = ivec2(node.size.x - node.size.x/2, node.size.y/2);
      child.center = node.center + ivec2(child.size.x/2,
                                         -child.size.y + child.size.y/2);
      child.index = 4*node.index + 2;
    }
  } else {
    if (tex_coord.x < node.center.x) {
      // bottom left
      child.size = ivec2(node.size.x/2, node.size.y - node.size.y/2);
      child.center = node.center + ivec2(-child.size.x + child.size.x/2,
                                         child.size.y/2);
      child.index = 4*node.index + 3;
    } else {
      // bottom right
      child.size = ivec2(node.size.x - node.size.x/2,
                         node.size.y - node.size.y/2);
      child.center = node.center + ivec2(child.size.x/2,
                                         child.size.y/2);
      child.index = 4*node.index + 4;
    }
  }
  child.level = node.level - 1;
  return child;
}

// bilinear sampling
void CDLODTerrain_bilinearSample(int base_offset, vec2 coord, ivec2 tex_size,
                                 out ivec4 offsets, out vec4 weights) {
  vec2 sp = coord * tex_size; // sample position in the texture

  // get the four nearest points
  ivec2 tl = ivec2(floor(sp.x), floor(sp.y));
  ivec2 tr = tl + ivec2(1, 0);
  ivec2 bl = tl + ivec2(0, 1);
  ivec2 br = tl + ivec2(1, 1);

  // calculate weights (works even in case of clamping)
  // Note: the weight scales with 1-distance, so basically
  // the weight of top left, is the distance of bottom right
  weights.x = (br.x - sp.x)*(br.y - sp.y); //tl
  weights.y = (sp.x - bl.x)*(bl.y - sp.y); //tr
  weights.z = (tr.x - sp.x)*(sp.y - tr.y); //bl
  weights.w = (sp.x - tl.x)*(sp.y - tl.y); //br

  // clamp to edge
  tl = min(tl, tex_size-1);
  tr = min(tr, tex_size-1);
  bl = min(bl, tex_size-1);
  br = min(br, tex_size-1);

  // calculate offsets
  offsets.x = base_offset + tl.y * tex_size.x + tl.x;
  offsets.y = base_offset + tr.y * tex_size.x + tr.x;
  offsets.z = base_offset + bl.y * tex_size.x + bl.x;
  offsets.w = base_offset + br.y * tex_size.x + br.x;
}

void CDLODTerrain_calculateOffset(CDLODTerrain_Node node, vec2 sample,
                                  out ivec4 offsets, out vec4 weights) {
  uvec4 data = texelFetch(CDLODTerrain_uHeightMapIndex, node.index);
  int base_offset = int((data.x << uint(16)) + data.y);
  ivec2 top_left = node.center - node.size/2;
  // the [0-1]x[0-1] coordinate of the sample in the node
  vec2 coord = (sample - vec2(top_left)) / vec2(node.size);
  ivec2 tex_size = ivec2(data.z, data.w);
  CDLODTerrain_bilinearSample(base_offset, coord, tex_size, offsets, weights);
}

float CDLODTerrain_fetchHeight(ivec4 offsets, vec4 weights) {
  float height = 0.0;
#ifndef NO_BILINEAR_SAMPLING
  for (int i = 0; i < 4; ++i) {
    height += texelFetch(CDLODTerrain_uHeightMap, offsets[i]).x * weights[i];
  }
#else
  height = texelFetch(CDLODTerrain_uHeightMap, offsets[0]).x;
#endif
  float scale = 100.0 / 255.0;
  return height * scale;
}

vec3 CDLODTerrain_worldPos(vec3 model_pos) {
  vec2 angles_degree = model_pos.xz * (vec2(360, 180) / CDLODTerrain_uTexSize);
  angles_degree = vec2(360-angles_degree.x, 180-angles_degree.y);
  vec2 angles = 1.000001f * angles_degree * M_PI / 180;
  float r = CDLODTerrain_uTexSize.x/2/M_PI + model_pos.y;
  vec3 cartesian = vec3(
    r*sin(angles.y)*cos(angles.x),
    -r*cos(angles.y),
    r*sin(angles.y)*sin(angles.x)
  );

  return cartesian;
}

float CDLODTerrain_estimateDistance(vec2 pos) {
  vec3 est_pos = vec3(pos.x, clamp(CDLODTerrain_cam_height, 0, 100), pos.y);
  vec3 est_diff = CDLODTerrain_uCamPos - CDLODTerrain_worldPos(est_pos);
  return length(est_diff);
}

float CDLODTerrain_getHeight(vec2 sample, float morph) {
  if (sample.x < 0 || CDLODTerrain_uTexSize.x < sample.x
      || sample.y < 0 || CDLODTerrain_uTexSize.y < sample.y) {
    return 0.0;
  } else {
    CDLODTerrain_Node node;
    // Root node
    node.center = CDLODTerrain_uTexSize/2;
    node.size = CDLODTerrain_uTexSize;
    node.level = CDLODTerrain_max_level;
    node.index = 0;

    float dist = CDLODTerrain_estimateDistance(sample);

    // Find the node that contains the given point (sample),
    // and its level is CDLODTerrain_uLevel.
    while (dist < length(vec2(node.size)) && node.level > 0) {
      node = CDLODTerrain_getChildOf(node, ivec2(sample));
    }

    ivec4 offsets;
    vec4 weights;
    CDLODTerrain_calculateOffset(node, sample, offsets, weights);
    return CDLODTerrain_fetchHeight(offsets, weights);
  }
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5) * 2.0;
  return (vertex - frac_part * morph);
}

vec4 CDLODTerrain_modelPos() {
  vec2 pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * CDLODTerrain_aPosition;

  float dist = CDLODTerrain_estimateDistance(pos);
  float max_dist = 2.35*pow(2.5, CDLODTerrain_uLevel) * CDLODTerrain_uNodeDimension;

  float start_dist = max(2.15/2.35*max_dist, max_dist - sqrt(max_dist));
  float dist_from_start = dist - start_dist;
  float start_to_end_dist = max_dist - start_dist;
  float morph = dist_from_start / start_to_end_dist;
  morph = clamp(morph, 0.0, 1.0);

  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  morphed_pos = CDLODTerrain_uOffset + CDLODTerrain_uScale * morphed_pos;
  float height = CDLODTerrain_getHeight(morphed_pos, morph);
  return vec4(morphed_pos.x, height, morphed_pos.y, morph);
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_uTexSize;
}
