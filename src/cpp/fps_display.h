// Copyright (c) 2015, Tamas Csala

#ifndef LOE_FPS_DISPLAY_H_
#define LOE_FPS_DISPLAY_H_

#include "engine/scene.h"
#include "engine/behaviour.h"
#include "engine/gui/label.h"

class FpsDisplay : public engine::Behaviour {
 public:
  explicit FpsDisplay(engine::GameObject* parent)
      : engine::Behaviour(parent), kRefreshInterval(0.1f) {
    using engine::gui::Font;

    fps_ = addComponent<engine::gui::Label>(
             L"FPS:",
             glm::vec2{0.9f, 0.9f},
             engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
             glm::vec4(1, 0, 0, 1)});
    fps_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    geom_nodes_ = addComponent<engine::gui::Label>(
               L"Geometry nodes:",
               glm::vec2{0.9f, 0.84f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 15,
               glm::vec4(1, 0, 0, 1)});
    geom_nodes_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    triangle_count_ = addComponent<engine::gui::Label>(
               L"Triangles count:",
               glm::vec2{0.9f, 0.80f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 15,
               glm::vec4(1, 0, 0, 1)});
    triangle_count_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    triangle_per_sec_ = addComponent<engine::gui::Label>(
               L"Triangles per sec:",
               glm::vec2{0.9f, 0.76f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 15,
               glm::vec4(1, 0, 0, 1)});
    triangle_per_sec_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    texture_nodes_ = addComponent<engine::gui::Label>(
               L"Texture nodes:",
               glm::vec2{0.9f, 0.70f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 15,
               glm::vec4(1, 0, 0, 1)});
    texture_nodes_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    memory_usage_ = addComponent<engine::gui::Label>(
               L"GPU memory usage:",
               glm::vec2{0.9f, 0.66f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 15,
               glm::vec4(1, 0, 0, 1)});
    memory_usage_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);
  }

  ~FpsDisplay() {
    std::cout << "FPS: "
      << min_fps_ << " min, "
      << sum_frame_num_ / sum_time_ << " avg, "
      << max_fps_ << " max" << std::endl;

    std::cout << "Triangles per frame: "
      << min_triangles_ << "K min, "
      << sum_triangle_num_ / sum_calls_ << "K avg, "
      << max_triangles_ << "K max" << std::endl;

    std::cout << "GPU Memory usage: "
      << min_memu_ << "MB min, "
      << sum_mem_usage_ / sum_calls_ << "MB avg, "
      << max_memu_ << "MB max" << std::endl;
  }

 private:
  engine::gui::Label *fps_;
  engine::gui::Label *geom_nodes_, *triangle_count_, *triangle_per_sec_;
  engine::gui::Label *texture_nodes_, *memory_usage_;
  const float kRefreshInterval;
  double sum_frame_num_ = 0, min_fps_ = 1.0/0.0, max_fps_ = 0;
  double sum_triangle_num_ = 0, min_triangles_ = 1.0/0.0, max_triangles_ = 0;
  double sum_mem_usage_ = 0, min_memu_ = 1.0/0.0, max_memu_ = 0;
  double sum_time_ = -0.1, sum_calls_ = 0, accum_time_ = 0, accum_calls_ = 0;

  virtual void update() override {
    sum_time_ += scene_->camera_time().dt;
    if (sum_time_ < 0.0) {
      return;
    }
    accum_time_ += scene_->camera_time().dt;
    accum_calls_++; sum_calls_++;

    double fps = accum_calls_ / accum_time_;
    size_t geom_nodes_count = engine::GlobalHeightMap::geom_nodes_count;
    size_t triangle_count = (geom_nodes_count
          << (2*engine::GlobalHeightMap::node_dimension_exp)) / 1000;
    size_t triangles_per_sec = triangle_count * fps / 1000;
    size_t gpu_mem_usage = engine::GlobalHeightMap::gpu_mem_usage/1024/1024;

    sum_frame_num_ += 1;
    min_fps_ = std::min(min_fps_, fps);
    max_fps_ = std::max(max_fps_, fps);
    sum_triangle_num_ += triangle_count;
    min_triangles_ = std::min<double>(min_triangles_, triangle_count);
    max_triangles_ = std::max<double>(max_triangles_, triangle_count);
    sum_mem_usage_ += gpu_mem_usage;
    min_memu_ = std::min<double>(min_memu_, gpu_mem_usage);
    max_memu_ = std::max<double>(max_memu_, gpu_mem_usage);

    if (accum_time_ > kRefreshInterval) {
      fps_->set_text(L"FPS: " + std::to_wstring(static_cast<int>(fps)));

      geom_nodes_->set_text(L"Geometry nodes: " +
        std::to_wstring(geom_nodes_count));

      triangle_count_->set_text(L"Triangles count: " +
        std::to_wstring(triangle_count) + L"K");

      triangle_per_sec_->set_text(L"Triangles per sec: " +
        std::to_wstring(triangles_per_sec) + L"M");

      texture_nodes_->set_text(L"Texture nodes: " +
        std::to_wstring(engine::GlobalHeightMap::texture_nodes_count));

      memory_usage_->set_text(L"GPU memory usage: " +
        std::to_wstring(gpu_mem_usage) + L"MB");

      accum_time_ = accum_calls_ = 0;
    }
  }
};

#endif
