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
uniform int CDLODTerrain_uGeomDiv;
ivec2 CDLODTerrain_GeomSize = CDLODTerrain_uTexSize << CDLODTerrain_uGeomDiv;
uniform vec3 CDLODTerrain_uCamPos;
uniform float CDLODTerrain_uNodeDimension;
uniform float CDLODTerrain_uLodLevelDistanceMultiplier;
int CDLODTerrain_uNodeDimensionExp = int(round(log2(CDLODTerrain_uNodeDimension)));

uniform int CDLODTerrain_max_level;
uniform int CDLODTerrain_max_height = 100;
float CDLODTerrain_height_scale = CDLODTerrain_max_height / 255.0;
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

float BellFunc(float x) {
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
      ivec2 pos = clamp(top_left + ivec2(x, y), ivec2(0), tex_size - ivec2(1));
      offsets[i] = base_offset + pos.y * tex_size.x + pos.x;

      float weight = BellFunc(x - fraction.x) * BellFunc(-y + fraction.y);
      weights[i] = weight;
      sum_weight += weight;
      i++;
    }
  }

  for (i = 0; i < 16; ++i) {
    weights[i] /= sum_weight;
  }
}

int CDLODTerrain_nearestSample(int base_offset, vec2 sample_pos, ivec2 tex_size) {
  ivec2 pos = clamp(ivec2(round(sample_pos)), ivec2(0), tex_size - ivec2(1));
  return base_offset + pos.y * tex_size.x + pos.x;
}

float CDLODTerrain_fetchHeight(int[16] offsets, float[16] weights) {
  float height = 0.0;
  for (int i = 0; i < 16; ++i) {
    height += texelFetch(CDLODTerrain_uHeightMap, offsets[i]).x * weights[i];
  }
  return height * CDLODTerrain_height_scale;
}

float CDLODTerrain_fetchHeight(int offset) {
  return texelFetch(CDLODTerrain_uHeightMap, offset).x * CDLODTerrain_height_scale;
}

float CDLODTerrain_fetchHeight(CDLODTerrain_Node node, vec2 tex_sample) {
  uvec4 data = texelFetch(CDLODTerrain_uHeightMapIndex, node.index);
  int base_offset = int((data.x << uint(16)) + data.y);
  ivec2 top_left = node.center - node.size/2;
  ivec2 tex_size = ivec2(data.z, data.w);

  vec2 sample_pos = (tex_sample - vec2(top_left)) * (tex_size / vec2(node.size));

  int[16] offsets;
  float[16] weights;
  CDLODTerrain_bicubicSample(base_offset, sample_pos, tex_size, offsets, weights);
  return CDLODTerrain_fetchHeight(offsets, weights);
}

float CDLODTerrain_fetchNearestHeight(CDLODTerrain_Node node, vec2 tex_sample) {
  uvec4 data = texelFetch(CDLODTerrain_uHeightMapIndex, node.index);
  int base_offset = int((data.x << uint(16)) + data.y);
  ivec2 top_left = node.center - node.size/2;
  ivec2 tex_size = ivec2(data.z, data.w);

  // invalid data
  if (tex_size.x == 0 || tex_size.y == 0) {
    return 0.0;
  }

  vec2 sample_pos = (tex_sample - vec2(top_left)) * (tex_size / vec2(node.size));

  int offset = CDLODTerrain_nearestSample(base_offset, sample_pos, tex_size);
  return CDLODTerrain_fetchHeight(offset);
}

vec3 CDLODTerrain_worldPos(vec3 model_pos) {
  vec2 angles = vec2(2*M_PI, M_PI) * (model_pos.xz / CDLODTerrain_GeomSize);
  float r = CDLODTerrain_radius + model_pos.y;
  vec3 cartesian = vec3(
    r*sin(angles.y)*cos(angles.x),
    r*cos(angles.y),
    -r*sin(angles.y)*sin(angles.x)
  );

  return cartesian;
}

bool CDLODTerrain_isValid(vec3 m_pos) {
  return 0 <= m_pos.x && m_pos.x <= CDLODTerrain_GeomSize.x &&
         0 <= m_pos.z && m_pos.z <= CDLODTerrain_GeomSize.y;
}

float CDLODTerrain_estimateDistance(vec2 geom_pos) {
  float est_height = clamp(CDLODTerrain_cam_height, 0, CDLODTerrain_max_height);
  vec3 est_pos = vec3(geom_pos.x, est_height, geom_pos.y);
  vec3 est_diff = CDLODTerrain_uCamPos - CDLODTerrain_worldPos(est_pos);
  return length(est_diff);
}

float CDLODTerrain_getHeight(vec2 geom_sample, out vec3 m_normal) {
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

    float height = CDLODTerrain_fetchHeight(node, tex_sample);

    // neighbours
    float diff = 1.0 / (1 << max(CDLODTerrain_uGeomDiv - node.level, 0));
    ivec2 node_top_left = node.center - node.size/2;
    mat3x3 neighbour_heights;
    for (int x = -1; x <= 1; ++x) {
      for (int y = -1; y <= 1; ++y) {
        vec2 neighbour = tex_sample + diff*vec2(x, y);
        // the [0-1]x[0-1] coordinate of the sample in the node
        vec2 coord = (neighbour - vec2(node_top_left)) / vec2(node.size);
        if (coord.x < 0 || node.size.x <= coord.x || coord.y < 0 ||
            node.size.y <= coord.y || (x == 0 && y == 0)) {
          neighbour_heights[x+1][y+1] = height;
        } else {
          neighbour_heights[x+1][y+1] = CDLODTerrain_fetchHeight(node, neighbour);
        }
      }
    }

    float gx = (neighbour_heights[2][0] + 2*neighbour_heights[2][1] + neighbour_heights[2][2]) -
               (neighbour_heights[0][0] + 2*neighbour_heights[0][1] + neighbour_heights[0][2]);

    float gy = (neighbour_heights[0][2] + 2*neighbour_heights[1][2] + neighbour_heights[2][2]) -
               (neighbour_heights[0][0] + 2*neighbour_heights[1][0] + neighbour_heights[2][0]);

    vec3 u = vec3(2*diff, gx, 0);
    vec3 v = vec3(0, gy, 2*diff);
    m_normal = normalize(cross(v, u));

    return height;
  }
}

vec2 CDLODTerrain_morphVertex(vec2 vertex, float morph) {
  vec2 frac_part = fract(vertex * 0.5) * 2.0;
  return (vertex - frac_part * morph);
}

vec2 CDLODTerrain_nodeLocal2Global(vec2 node_coord, float scale) {
  vec2 pos = CDLODTerrain_uOffset + scale * node_coord;
  return clamp(pos, vec2(0, 0), CDLODTerrain_GeomSize);
}

vec4 CDLODTerrain_modelPos(out vec3 m_normal) {
  float scale = CDLODTerrain_uScale;
  vec2 pos = CDLODTerrain_nodeLocal2Global(CDLODTerrain_aPosition, scale);
  int iteration_count = 0;

  float dist = CDLODTerrain_estimateDistance(pos);
  float next_level_size = (1 << (CDLODTerrain_uLevel+1))
                          * CDLODTerrain_uLodLevelDistanceMultiplier
                          * CDLODTerrain_uNodeDimension;
  float max_dist = 0.85*next_level_size;
  float start_dist = 0.75*next_level_size;
  float morph = smoothstep(start_dist, max_dist, dist);

  vec2 morphed_pos = CDLODTerrain_morphVertex(CDLODTerrain_aPosition, morph);
  pos = CDLODTerrain_nodeLocal2Global(morphed_pos, scale);
  dist = CDLODTerrain_estimateDistance(pos);

  while (dist > 1.5*next_level_size && (iteration_count+1 < CDLODTerrain_uNodeDimensionExp)) {
    scale *= 2;
    next_level_size *= 2;
    iteration_count += 1;
    max_dist = 0.85*next_level_size;
    start_dist = 0.75*next_level_size;
    morph = smoothstep(start_dist, max_dist, dist);
    if (morph == 0.0) {
      break;
    }

    float sc = 1 << iteration_count;
    vec2 morphed_offset = CDLODTerrain_morphVertex(CDLODTerrain_uOffset / sc, morph) * sc;
    vec2 offset_error = CDLODTerrain_uOffset - morphed_offset;
    morphed_pos = CDLODTerrain_morphVertex(morphed_pos * 0.5, morph);
    pos = offset_error + CDLODTerrain_nodeLocal2Global(morphed_pos, scale);
    dist = CDLODTerrain_estimateDistance(pos);
  }

  float height = CDLODTerrain_getHeight(pos, m_normal);
  return vec4(pos.x, height, pos.y, iteration_count + morph);
}

vec2 CDLODTerrain_texCoord(vec3 pos) {
  return pos.xz / CDLODTerrain_GeomSize;
}
