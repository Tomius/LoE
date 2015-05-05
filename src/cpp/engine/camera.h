// Copyright (c) 2015, Tamas Csala

#ifndef ENGINE_CAMERA_H_
#define ENGINE_CAMERA_H_

#include <cmath>

#define GLM_FORCE_RADIANS
#include <glm/gtx/rotate_vector.hpp>

#include "./timer.h"
#include "./behaviour.h"
#include "collision/frustum.h"

namespace engine {

class CameraTransform : public Transform {
 public:
  CameraTransform() : up_(vec3{0, 1, 0}) {}

  // We shouldn't inherit the parent's rotation, like how a normal Transform does
  virtual const quat rot() const override { return rot_; }
  virtual void set_rot(const quat& new_rot) override { rot_ = new_rot; }

  // We have custom up and right vectors
  virtual vec3 up() const override { return up_; }
  virtual void set_up(const vec3& new_up) override { up_ = glm::normalize(new_up); }
  virtual vec3 right() const override {
    return glm::normalize(glm::cross(forward(), up()));
  }

  virtual void set_right(const vec3& new_right) override {
    set_forward(glm::cross(up(), new_right));
  }

 private:
  vec3 up_;
};

/// The base class for all cameras
class Camera : public Behaviour {
 public:
  Camera(GameObject* parent, double fovy, double z_near, double z_far)
      : Behaviour(parent, CameraTransform{}), fovy_(fovy), z_near_(z_near)
      , z_far_(z_far), width_(0), height_(0) { }
  virtual ~Camera() {}

  virtual void screenResized(size_t width, size_t height) override {
    width_ = width;
    height_ = height;
  }

  const glm::dmat4& cameraMatrix() const { return cam_mat_; }
  const glm::dmat4& projectionMatrix() const { return proj_mat_; }
  const Frustum& frustum() const { return frustum_; }

  double fovx() const { return fovy_*width_/height_;}
  void set_fovx(double fovx) { fovy_ = fovx*height_/width_; }
  double fovy() const { return fovy_;}
  void set_fovy(double fovy) { fovy_ = fovy; }
  double z_near() const { return z_near_;}
  void set_z_near(double z_near) { z_near_ = z_near; }
  double z_far() const { return z_far_;}
  void set_z_far(double z_far) { z_far_ = z_far; }

  bool isPointInsideFrustum(const glm::dvec3& p) const {
    glm::dmat4 mat = projectionMatrix() * cameraMatrix();
    glm::dvec4 proj = mat * glm::dvec4(p, 1);
    proj /= proj.w;

    return -1 < proj.x && proj.x < 1 && -1 < proj.y && proj.y < 1 &&
            0 < proj.z && proj.z < 1;
  }

 protected:
  // it must be called through update()
  void update_cache() {
    updateCameraMatrix();
    updateProjectionMatrix();
    updateFrustum();
  }

 private:
  double fovy_, z_near_, z_far_, width_, height_;

  glm::dmat4 cam_mat_, proj_mat_;
  Frustum frustum_;

  void updateCameraMatrix() {
    const Transform* t = transform();
    cam_mat_ = glm::lookAt(t->pos(), t->pos()+t->forward(), t->up());
  }

  void updateProjectionMatrix() {
    proj_mat_ = glm::perspectiveFov<double>(fovy_, width_, height_, z_near_, z_far_);
  }

  void updateFrustum() {
    glm::dmat4 m = proj_mat_ * cam_mat_;

    // REMEMBER: m[i][j] is j-th row, i-th column!!!

    frustum_ = Frustum{{
      // left
     {m[0][3] + m[0][0],
      m[1][3] + m[1][0],
      m[2][3] + m[2][0],
      m[3][3] + m[3][0]},

      // right
     {m[0][3] - m[0][0],
      m[1][3] - m[1][0],
      m[2][3] - m[2][0],
      m[3][3] - m[3][0]},

      // top
     {m[0][3] - m[0][1],
      m[1][3] - m[1][1],
      m[2][3] - m[2][1],
      m[3][3] - m[3][1]},

      // bottom
     {m[0][3] + m[0][1],
      m[1][3] + m[1][1],
      m[2][3] + m[2][1],
      m[3][3] + m[3][1]},

      // near
     {m[0][2],
      m[1][2],
      m[2][2],
      m[3][2]},

      // far
     {m[0][3] - m[0][2],
      m[1][3] - m[1][2],
      m[2][3] - m[2][2],
      m[3][3] - m[3][2]}
    }};

    // Note: there's no need to normalize the plane parameters
  }
};

class FreeFlyCamera : public Camera {
 public:
  FreeFlyCamera(GameObject* parent, double fov, double z_near,
                double z_far, const glm::dvec3& pos,
                const glm::dvec3& target = glm::dvec3(),
                double speed_per_sec = 5.0f,
                double mouse_sensitivity = 1.0f)
      : Camera(parent, fov, z_near, z_far)
      , first_call_(true)
      , speed_per_sec_(speed_per_sec)
      , mouse_sensitivity_(mouse_sensitivity)
      , cos_max_pitch_angle_(0.98f) {
    transform()->set_pos(pos);
    transform()->set_forward(target - pos);
  }

  double speed_per_sec() const { return speed_per_sec_; }
  double mouse_sensitivity() const { return mouse_sensitivity_; }
  double cos_max_pitch_angle() const { return cos_max_pitch_angle_; }

  void set_speed_per_sec(double value) { speed_per_sec_ = value; }
  void set_mouse_sensitivity(double value) { mouse_sensitivity_ = value; }
  void set_cos_max_pitch_angle(double value) { cos_max_pitch_angle_ = value; }

 protected:
  bool first_call_;
  double speed_per_sec_, mouse_sensitivity_, cos_max_pitch_angle_;

 private:
  virtual void update() override;
};

class ThirdPersonalCamera : public Camera {
 public:
  ThirdPersonalCamera(GameObject* parent,
                      double fov,
                      double z_near,
                      double z_far,
                      const glm::dvec3& position,
                      double mouse_sensitivity = 1.0f,
                      double mouse_scroll_sensitivity = 1.0f,
                      double min_dist_mod = 0.25f,
                      double max_dist_mod = 4.00f,
                      double dist_offset = 0.0f)
      : Camera(parent, fov, z_near, z_far)
      , target_(parent->transform())
      , first_call_(true)
      , curr_dist_mod_(1.0f)
      , dest_dist_mod_(1.0f)
      , initial_distance_(glm::length(target_->pos() - position) - dist_offset)
      , cos_max_pitch_angle_(0.98f)
      , mouse_sensitivity_(mouse_sensitivity)
      , mouse_scroll_sensitivity_(mouse_scroll_sensitivity)
      , min_dist_mod_(min_dist_mod)
      , max_dist_mod_(max_dist_mod)
      , dist_offset_(dist_offset) {
    transform()->set_pos(position);
    transform()->set_forward(target_->pos() - position);
  }

  virtual ~ThirdPersonalCamera() {}

 private:
  // The target object's transform, that the camera is following
  Transform *target_;

  // We shouldn't interpolate at the first call.
  bool first_call_;

  // For mouseScrolled interpolation
  double curr_dist_mod_, dest_dist_mod_;

  // Private constant numbers
  const double initial_distance_, cos_max_pitch_angle_,
               mouse_sensitivity_, mouse_scroll_sensitivity_,
               min_dist_mod_, max_dist_mod_, dist_offset_;

  virtual void update() override;

  virtual void mouseScrolled(double, double yoffset) override {
    dest_dist_mod_ -= yoffset / 4.0f * mouse_scroll_sensitivity_;
    if (dest_dist_mod_ < min_dist_mod_) {
      dest_dist_mod_ = min_dist_mod_;
    } else if (dest_dist_mod_ > max_dist_mod_) {
      dest_dist_mod_ = max_dist_mod_;
    }
  }
};  // ThirdPersonalCamera

}  // namespace engine

#endif  // ENGINE_CAMERA_H_
