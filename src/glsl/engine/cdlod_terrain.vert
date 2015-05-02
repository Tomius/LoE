#version 330

#export vec4 CDLODTerrain_modelPos();
#export vec3 CDLODTerrain_worldPos(vec3 model_pos);
#export vec2 CDLODTerrain_texCoord(vec3 pos);
#export bool CDLODTerrain_isValid(vec3 m_pos);
#export bool CDLODTerrain_isVisible(vec3 m_pos);
#export float CDLODTerrain_getHeight(vec2 sample);

in vec2 CDLODTerrain_aPosition;

// Vertex attrib divisor works like a uniform
in vec4 CDLODTerrain_uRenderData;

vec2 CDLODTerrain_uOffset = CDLODTerrain_uRenderData.xy;
float CDLODTerrain_uScale = CDLODTerrain_uRenderData.z;
int CDLODTerrain_uLevel = int(CDLODTerrain_uRenderData.w);

uniform ivec2 CDLODTerrain_uTexSize;
uniform int CDLODTerrain_uGeomDiv;
ivec2 CDLODTerrain_GeomSize = CDLODTerrain_uTexSize << CDLODTerrain_uGeomDiv;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;

uniform int CDLODTerrain_max_level;
uniform int CDLODTerrain_max_height = 100;
uniform usamplerBuffer CDLODTerrain_uHeightMap;
uniform usamplerBuffer CDLODTerrain_uHeightMapIndex;

struct CDLODTerrain_Node {
  ivec2 center, size;
  int level, index;
};

float M_PI = 3.14159265359;

float CDLODTerrain_radius = CDLODTerrain_GeomSize.x/2/M_PI;
float CDLODTerrain_cam_height = length(CDLODTerrain_uCamPos) - CDLODTerrain_radius;

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
  for (int i = 0; i < 4; ++i) {
    height += texelFetch(CDLODTerrain_uHeightMap, offsets[i]).x * weights[i];
  }
  float scale = CDLODTerrain_max_height / 255.0;
  return height * scale;
}

vec3 CDLODTerrain_worldPos(vec3 model_pos) {
  vec2 angles_degree = vec2(360, 180) * (model_pos.xz / CDLODTerrain_GeomSize);
  angles_degree = vec2(360-angles_degree.x, 180-angles_degree.y);
  vec2 angles = angles_degree * M_PI / 180;
  float r = CDLODTerrain_radius + model_pos.y;
  vec3 cartesian = vec3(
    r*sin(angles.y)*cos(angles.x),
    -r*cos(angles.y),
    r*sin(angles.y)*sin(angles.x)
  );

  return cartesian;
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_GeomSize;
}

bool CDLODTerrain_isValid(vec3 m_pos) {
  return 0 <= m_pos.x && m_pos.x <= CDLODTerrain_GeomSize.x &&
         0 <= m_pos.z && m_pos.z <= CDLODTerrain_GeomSize.y;
}

bool CDLODTerrain_isVisible(vec3 m_pos) {
  int border = 1 << (CDLODTerrain_uLevel+1);
  return -border <= m_pos.x && m_pos.x <= CDLODTerrain_GeomSize.x + border &&
         -border <= m_pos.z && m_pos.z <= CDLODTerrain_GeomSize.y + border;
}

float CDLODTerrain_estimateDistance(vec2 geom_pos) {
  float est_height = clamp(CDLODTerrain_cam_height, 0, CDLODTerrain_max_height);
  vec3 est_pos = vec3(geom_pos.x, est_height, geom_pos.y);
  vec3 est_diff = CDLODTerrain_uCamPos - CDLODTerrain_worldPos(est_pos);
  return length(est_diff);
}

float CDLODTerrain_getHeight(vec2 geom_sample) {
  if (!CDLODTerrain_isValid(vec3(geom_sample.x, 0, geom_sample.y))) {
    return 0.0;
  } else {
    CDLODTerrain_Node node;
    // Root node
    node.center = CDLODTerrain_uTexSize/2;
    node.size = CDLODTerrain_uTexSize;
    node.level = CDLODTerrain_max_level;
    node.index = 0;

    float dist = CDLODTerrain_estimateDistance(geom_sample);

    vec2 tex_sample = geom_sample / (1 << CDLODTerrain_uGeomDiv);

    // Find the node that contains the given point (tex_sample).
    while (dist < length(vec2(node.size)) && node.level > 0) {
      node = CDLODTerrain_getChildOf(node, ivec2(tex_sample));
    }

    ivec4 offsets;
    vec4 weights;
    CDLODTerrain_calculateOffset(node, tex_sample, offsets, weights);
    return CDLODTerrain_fetchHeight(offsets, weights);
  }
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5) * 2.0;
  return (vertex - frac_part * morph);
}

vec4 CDLODTerrain_modelPos() {
  float scale = CDLODTerrain_uScale;
  vec2 pos = CDLODTerrain_uOffset + scale * CDLODTerrain_aPosition;

  float dist = CDLODTerrain_estimateDistance(pos);
  float next_border = (1 << (CDLODTerrain_uLevel+1)) * CDLODTerrain_uNodeDimension;

  float max_dist = 0.9*next_border;
  float start_dist = max(0.95*max_dist, max_dist - sqrt(max_dist));
  float dist_from_start = dist - start_dist;
  float start_to_end_dist = max_dist - start_dist;
  float morph = dist_from_start / start_to_end_dist;
  morph = clamp(morph, 0.0, 1.0);
  int morph_base = 0;

  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  pos = CDLODTerrain_uOffset + scale * morphed_pos;
  dist = CDLODTerrain_estimateDistance(pos);

  while (dist > 1.5*next_border) {
    scale *= 2;
    next_border *= 2;
    morph_base += 1;
    max_dist = 0.9*next_border;
    start_dist = max(0.95*max_dist, max_dist - sqrt(max_dist));
    dist_from_start = dist - start_dist;
    start_to_end_dist = max_dist - start_dist;
    morph = dist_from_start / start_to_end_dist;
    morph = clamp(morph, 0.0, 1.0);

    morphed_pos = CDLODTerrain_morphVertex(morphed_pos/2.0, morph);

    pos = CDLODTerrain_uOffset + scale * morphed_pos;
    dist = CDLODTerrain_estimateDistance(pos);
  }

  float height = CDLODTerrain_getHeight(pos);
  return vec4(pos.x, height, pos.y, (morph_base+morph)/3.0);
}
