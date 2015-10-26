#version 330

#export vec4 CDLODTerrain_modelPos(out vec3 m_normal);
#export vec3 CDLODTerrain_worldPos(vec3 model_pos);
#export vec2 CDLODTerrain_texCoord(vec3 pos);
#export bool CDLODTerrain_isValid(vec3 m_pos);
#export float CDLODTerrain_getHeight(vec2 sample, out vec3 m_normal);

in vec2 CDLODTerrain_aPosition;
in vec4 CDLODTerrain_aRenderData;

vec2 CDLODTerrain_uOffset = CDLODTerrain_aRenderData.xy;
float CDLODTerrain_uScale = CDLODTerrain_aRenderData.z;
int CDLODTerrain_uLevel = int(CDLODTerrain_aRenderData.w);

uniform ivec2 CDLODTerrain_uTexSize;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;
uniform float CDLODTerrain_uLodLevelDistanceMultiplier;
int CDLODTerrain_uNodeDimensionExp = int(round(log2(CDLODTerrain_uNodeDimension)));

uniform int CDLODTerrain_max_level;
uniform int CDLODTerrain_max_height;
float CDLODTerrain_height_scale = CDLODTerrain_max_height / ((1 << 16) - 1.0);
uniform usamplerBuffer CDLODTerrain_uHeightMap;
uniform usamplerBuffer CDLODTerrain_uNormalMap;
uniform usamplerBuffer CDLODTerrain_uIndexTexture;

struct CDLODTerrain_Node {
  ivec2 center, size;
  int level, index;
  uvec4 data;
};

float M_PI = 3.14159265359;
ivec2 kBorderSize = ivec2(2, 2);
const float morph_end = 0.95, morph_start = 0.8;

float CDLODTerrain_radius = CDLODTerrain_uTexSize.x / 2 / M_PI;
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

int CDLODTerrain_CalculateOffset(int base_offset, ivec2 tex_size, ivec2 pos) {
  return base_offset + (pos.y + kBorderSize.y) * tex_size.x + (pos.x + kBorderSize.x);
}

float CDLODTerrain_BellFunc(float x) {
  float f = x * 0.75; // Converting -2 to +2 to -1.5 to +1.5
  if (f > -1.5 && f < -0.5) {
    return 0.5 * pow(f + 1.5, 2.0);
  } else if (f > -0.5 && f < 0.5) {
    return 3.0 / 4.0 - (f*f);
  } else if (f > 0.5 && f < 1.5) {
    return 0.5 * pow(f - 1.5, 2.0);
  } else {
    return 0.0;
  }
}

void CDLODTerrain_bicubicSample(int base_offset, vec2 sample_pos, ivec2 tex_size,
                                out int[16] offsets, out float[16] weights) {

  ivec2 top_left = ivec2(floor(sample_pos));
  vec2 fraction = fract(sample_pos);

  int i = 0;
  float sum_weight = 0.0;
  for (int x = -1; x <= 2; x++) {
    for (int y = -1; y <= 2; y++) {
      ivec2 pos = clamp(top_left + ivec2(x, y), ivec2(-2), tex_size + ivec2(1));
      offsets[i] = CDLODTerrain_CalculateOffset(base_offset, tex_size, pos);

      float weight = CDLODTerrain_BellFunc(x - fraction.x) * CDLODTerrain_BellFunc(-y + fraction.y);
      weights[i] = weight;
      sum_weight += weight;
      i++;
    }
  }

  for (i = 0; i < 16; ++i) {
    weights[i] /= sum_weight;
  }
}

float CDLODTerrain_fetchHeight(int[16] offsets, float[16] weights) {
  float height = 0.0;
  for (int i = 0; i < 16; ++i) {
    height += texelFetch(CDLODTerrain_uHeightMap, offsets[i]).x * weights[i];
  }
  return height * CDLODTerrain_height_scale;
}

bool CDLODTerrain_getOffsetsAndWeights(CDLODTerrain_Node node, vec2 tex_sample,
                                       out int[16] offsets, out float[16] weights) {
  int base_offset = int((node.data.x << uint(16)) + node.data.y);
  ivec2 top_left = node.center - node.size/2;
  ivec2 tex_size = ivec2(node.data.z, node.data.w);

  // invalid data
  if (tex_size.x == 0 || tex_size.y == 0) {
    return false;
  }

  vec2 sample_pos = (tex_sample - vec2(top_left)) * ((tex_size-2*kBorderSize) / vec2(node.size));

  CDLODTerrain_bicubicSample(base_offset, sample_pos, tex_size, offsets, weights);

  return true;
}

vec3 CDLODTerrain_fetchNormal(int level, int[16] offsets, float[16] weights) {
  float dx = 0.0, dy = 0.0;
  for (int i = 0; i < 16; ++i) {
    vec4 texel = texelFetch(CDLODTerrain_uNormalMap, offsets[i]) * weights[i];
    dx += texel.x;
    dy += texel.y;
  }
  dx *= CDLODTerrain_height_scale;
  dy *= CDLODTerrain_height_scale;

  float real_world_diff = 1 << level;
  vec3 u = vec3(real_world_diff, dx, 0);
  vec3 v = vec3(0, dy, real_world_diff);
  return normalize(cross(v, u));
}

void CDLODTerrain_fetchHeightAndNormal(CDLODTerrain_Node node,
                                       vec2 tex_sample,
                                       out float height,
                                       out vec3 normal) {
  int[16] offsets;
  float[16] weights;
  bool success = CDLODTerrain_getOffsetsAndWeights(node, tex_sample, offsets, weights);
  if (success) {
    height = CDLODTerrain_fetchHeight(offsets, weights);
    normal = CDLODTerrain_fetchNormal(node.level, offsets, weights);
  } else {
    normal = vec3(0, 1, 0);
    height = 0;
  }
}

vec3 CDLODTerrain_worldPos(vec3 model_pos) {
  vec2 angles = vec2(2*M_PI, M_PI) * (model_pos.xz / CDLODTerrain_uTexSize);
  float r = CDLODTerrain_radius + model_pos.y;
  vec3 cartesian = vec3(
    r*sin(angles.y)*cos(angles.x),
    r*cos(angles.y),
    -r*sin(angles.y)*sin(angles.x)
  );

  return cartesian;
}

bool CDLODTerrain_isValid(vec3 m_pos) {
  return 0 <= m_pos.x && m_pos.x <= CDLODTerrain_uTexSize.x &&
         0 <= m_pos.z && m_pos.z <= CDLODTerrain_uTexSize.y;
}

float CDLODTerrain_estimateDistance(vec2 geom_pos) {
  float est_height = clamp(CDLODTerrain_cam_height, 0, CDLODTerrain_max_height);
  vec3 est_pos = vec3(geom_pos.x, est_height, geom_pos.y);
  vec3 est_diff = CDLODTerrain_uCamPos - CDLODTerrain_worldPos(est_pos);
  return length(est_diff);
}

void CDLODTerrain_getHeightAndNormal(vec2 tex_sample,
                                     float morph,
                                     out float height,
                                     out vec3 normal) {
  if (!CDLODTerrain_isValid(vec3(tex_sample.x, 0, tex_sample.y))) {
    height = 0.0;
    normal = vec3(0, 1, 0);
    return;
  }

  // Root node
  CDLODTerrain_Node node;
  node.center = CDLODTerrain_uTexSize/2;
  node.size = CDLODTerrain_uTexSize;
  node.level = CDLODTerrain_max_level;
  node.index = 0;

  float dist = CDLODTerrain_estimateDistance(tex_sample);

  CDLODTerrain_Node last_node = node;

  // Find the node that contains the given point (tex_sample).
  while (0 < node.level && dist < node.size.x) {
    last_node = node;
    CDLODTerrain_Node child = CDLODTerrain_getChildOf(node, ivec2(tex_sample));
    child.data = texelFetch(CDLODTerrain_uIndexTexture, child.index);
    if (child.data.z == uint(0)) {
      break;
    } else {
      node = child;
    }
  }

  int fetch_count;
  CDLODTerrain_Node nodes[2];
  vec3 normals[2];
  float heights[2];

  int next_tex_lod_level = 2*node.size.x;
  float max_dist = morph_end * next_tex_lod_level;
  float start_dist = morph_start * next_tex_lod_level;
  float normal_morph = smoothstep(start_dist, max_dist, dist);

  if (normal_morph == 0 || CDLODTerrain_uLevel < 0) {
    fetch_count = 1;
    nodes[0] = node;
  } else if (normal_morph == 1) {
    fetch_count = 1;
    nodes[0] = last_node;
  } else {
    fetch_count = 2;
    nodes[0] = node;
    nodes[1] =  last_node;
  }

  for (int i = 0; i < fetch_count; ++i) {
    CDLODTerrain_fetchHeightAndNormal(nodes[i], tex_sample, heights[i], normals[i]);
  }

  if (fetch_count == 1) {
    height = heights[0];
    normal = normals[0];
  } else {
    height = mix(heights[0], heights[1], normal_morph);
    normal = mix(normals[0], normals[1], normal_morph);
  }
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5) * 2.0;
  return (vertex - frac_part * morph);
}

vec2 CDLODTerrain_nodeLocal2Global(vec2 node_coord, float scale) {
  vec2 pos = CDLODTerrain_uOffset + scale * node_coord;
  return clamp(pos, vec2(0, 0), CDLODTerrain_uTexSize);
}

vec4 CDLODTerrain_modelPos(out vec3 m_normal) {
  float scale = CDLODTerrain_uScale;
  vec2 pos = CDLODTerrain_nodeLocal2Global(CDLODTerrain_aPosition, scale);
  int iteration_count = 0;

  float dist = CDLODTerrain_estimateDistance(pos);
  float next_level_size = pow(2, CDLODTerrain_uLevel+1)
                          * CDLODTerrain_uLodLevelDistanceMultiplier
                          * CDLODTerrain_uNodeDimension;
  float max_dist = morph_end * next_level_size;
  float start_dist = morph_start * next_level_size;
  float morph = smoothstep(start_dist, max_dist, dist);

  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  pos = CDLODTerrain_nodeLocal2Global(morphed_pos, scale);
  dist = CDLODTerrain_estimateDistance(pos);

  while (dist > 1.5*next_level_size && (iteration_count+1 < CDLODTerrain_uNodeDimensionExp)) {
    scale *= 2;
    next_level_size *= 2;
    iteration_count += 1;
    max_dist = morph_end * next_level_size;
    start_dist = morph_start * next_level_size;
    morph = smoothstep(start_dist, max_dist, dist);
    if (morph == 0.0) {
      break;
    }

    morphed_pos = CDLODTerrain_morphVertex(morphed_pos * 0.5, morph);
    pos = CDLODTerrain_nodeLocal2Global(morphed_pos, scale);
    dist = CDLODTerrain_estimateDistance(pos);
  }

  float height;
  CDLODTerrain_getHeightAndNormal(pos, iteration_count + morph, height, m_normal);
  return vec4(pos.x, height, pos.y, iteration_count + morph);
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_uTexSize;
}

