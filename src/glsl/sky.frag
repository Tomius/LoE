// Copyright (c) 2015, Tamas Csala

#version 330

#export vec3 SkyColor(vec3 look_dir);
#export vec3 SunPos();
#export float SunPower();
#export float AmbientPower();
#export vec3 SunColor();
#export vec3 AmbientColor();

uniform vec3 uSunPos;
vec3 sun_pos = normalize(uSunPos);

vec3 SkyColor(vec3 look_dir) {
  float look_dir_sun_dist = max(dot(look_dir, sun_pos), 0.0);
  float l = look_dir_sun_dist;

  vec3 sun_color = vec3(0.97, 0.95, 0.2);

  vec3 sun = vec3(0.0);
  float scale = 0.0;
  for (int i = 8; i < 64; i += 4) {
    float curr_scale = pow(1.32, float(i));
    scale += curr_scale;
    sun += curr_scale * pow(sun_color, vec3(pow(l, -i*25)));
  }

  return 3 * sun / scale;
}

// Functions for other objects' lighting computations
vec3 SunPos() { return sun_pos; }
float SunPower() { return clamp(sun_pos.y, 0, 1); }
vec3 SunColor() { return vec3(1.0, 0.9, 0.75); }

float AmbientPower() { return 0.25 * max(SunPower(), 0.3); }
vec3 AmbientColor() {
  return SunPower() * SunColor();
}
