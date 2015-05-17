// Copyright (c) 2015, Tamas Csala
// This shader is based on an oglplus example:
// http://oglplus.org/oglplus/html/oglplus_2023_sky_8cpp-example.html

#version 330

#export vec3 SkyColor(vec3 look_dir);
#export vec3 SunPos();
#export float SunPower();
#export float AmbientPower();
#export vec3 SunColor();
#export vec3 AmbientColor();

const float kWorldRadius = 6371000;
const float kAtmThickness = 50000;
const vec3 kAirColor = vec3(0.32, 0.36, 0.45);
const vec3 kLightColor = vec3(1.0, 1.0, 1.0);

uniform vec3 uSunPos;
vec3 sun_pos = normalize(uSunPos);
vec3 moon_pos = -sun_pos;

float sky_sqr(float x) {
  return x*x;
}

vec3 SkyColor(vec3 look_dir) {
  float look_dir_sun_dist = max(dot(look_dir, sun_pos), 0.0);

  float airLightScale = 1 + (look_dir_sun_dist+1)/4;
  vec3 sun = 0.6 * pow(vec3(0.93, 0.91, 0.4), vec3(pow(look_dir_sun_dist, -256)));

  return clamp(sun + airLightScale*kAirColor, 0.0, 1.0);
}

// Functions for other objects' lighting computations
vec3 SunPos() { return sun_pos; }
float SunPower() { return clamp(sun_pos.y, 0, 1); }
vec3 SunColor() { return vec3(1.0, 0.9, 0.75); }

float AmbientPower() { return 0.25 * max(SunPower(), 0.3); }
vec3 AmbientColor() {
  return SunPower() * SunColor();
}
