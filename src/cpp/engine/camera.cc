// Copyright (c) 2015, Tamas Csala

#include "./camera.h"
#include "./scene.h"

namespace engine {

void FreeFlyCamera::update() {
  glm::dvec2 cursor_pos;
  GLFWwindow* window = scene_->window();
  glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
  static glm::dvec2 prev_cursor_pos;
  glm::dvec2 diff = cursor_pos - prev_cursor_pos;
  prev_cursor_pos = cursor_pos;

  // We get invalid diff values at the startup
  if (first_call_) {
    diff = glm::dvec2(0, 0);
    first_call_ = false;
  }

  const float dt = scene_->camera_time().dt;

  // Mouse movement - update the coordinate system
  if (diff.x || diff.y) {
    float dx(diff.x * mouse_sensitivity_ * dt / 16);
    float dy(-diff.y * mouse_sensitivity_ * dt / 16);

    // If we are looking up / down, we don't want to be able
    // to rotate to the other side
    float dot_up_fwd = glm::dot(transform()->up(), transform()->forward());
    if (dot_up_fwd > cos_max_pitch_angle_ && dy > 0) {
      dy = 0;
    }
    if (dot_up_fwd < -cos_max_pitch_angle_ && dy < 0) {
      dy = 0;
    }

    transform()->set_forward(transform()->forward() +
                             transform()->right()*dx +
                             transform()->up()*dy);
  }

  // Update the position
  float ds = dt * speed_per_sec_;
  glm::vec3 local_pos = transform()->local_pos();
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    local_pos += transform()->forward() * ds;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    local_pos -= transform()->forward() * ds;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    local_pos += transform()->right() * ds;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    local_pos -= transform()->right() * ds;
  }
  transform()->set_local_pos(local_pos);

  update_cache();
}

void ThirdPersonalCamera::update() {
  static glm::dvec2 prev_cursor_pos;
  glm::dvec2 cursor_pos;
  GLFWwindow* window = scene_->window();
  glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
  glm::dvec2 diff = cursor_pos - prev_cursor_pos;
  prev_cursor_pos = cursor_pos;

  // We get invalid diff values at the startup
  if (first_call_) {
    diff = glm::dvec2(0, 0);
    first_call_ = false;
  }

  const float dt = scene_->camera_time().dt;

  // Mouse movement - update the coordinate system
  if (diff.x || diff.y) {
    float mouse_sensitivity = mouse_sensitivity_ * curr_dist_mod_ * dt / 64;
    float dx(diff.x * mouse_sensitivity);
    float dy(-diff.y * mouse_sensitivity);

    // If we are looking up / down, we don't want to be able
    // to rotate to the other side
    float dot_up_fwd = glm::dot(transform()->up(), transform()->forward());
    if (dot_up_fwd > cos_max_pitch_angle_ && dy > 0) {
      dy = 0;
    }
    if (dot_up_fwd < -cos_max_pitch_angle_ && dy < 0) {
      dy = 0;
    }

    transform()->set_forward(transform()->forward() +
                             transform()->right()*dx +
                             transform()->up()*dy);
  }

  float dist_diff_mod = dest_dist_mod_ - curr_dist_mod_;
  if (fabs(dist_diff_mod) > dt * 2 * mouse_scroll_sensitivity_) {
    int sign = dist_diff_mod / fabs(dist_diff_mod);
    curr_dist_mod_ += sign * dt * 2 * mouse_scroll_sensitivity_;
  }

  // Update the position
  glm::vec3 tpos(target_->pos()), fwd(transform()->forward());
  glm::vec3 pos(tpos - fwd*curr_dist_mod_*initial_distance_);

  fwd = transform()->forward();
  pos = tpos - fwd*curr_dist_mod_*initial_distance_;
  transform()->set_pos(pos);

  update_cache();
}

}  // namespace engine
