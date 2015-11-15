// Copyright (c) 2015, Tamas Csala

#ifndef LOE_FPS_DISPLAY_H_
#define LOE_FPS_DISPLAY_H_

#include "engine/scene.h"
#include "engine/behaviour.h"
#include "engine/gui/label.h"

class FpsDisplay : public engine::Behaviour {
 public:
  explicit FpsDisplay(engine::GameObject* parent)
      : engine::Behaviour(parent), kRefreshInterval(0.1f)
      , sum_frame_num_(0), sum_time_(0) {
    using engine::gui::Font;

    fps_ = addComponent<engine::gui::Label>(
             L"FPS:",
             glm::vec2{0.9f, 0.9f},
             engine::gui::Font{"src/resources/fonts/Vera.ttf", 25,
             glm::vec4(1, 0, 0, 1)});
    fps_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    geom_nodes_ = addComponent<engine::gui::Label>(
               L"Geometry nodes:",
               glm::vec2{0.9f, 0.80f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
               glm::vec4(1, 0, 0, 1)});
    geom_nodes_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    triangle_count_ = addComponent<engine::gui::Label>(
               L"Triangles count:",
               glm::vec2{0.9f, 0.75f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
               glm::vec4(1, 0, 0, 1)});
    triangle_count_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    triangle_per_sec_ = addComponent<engine::gui::Label>(
               L"Triangles per sec:",
               glm::vec2{0.9f, 0.70f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
               glm::vec4(1, 0, 0, 1)});
    triangle_per_sec_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    texture_nodes_ = addComponent<engine::gui::Label>(
               L"Texture nodes:",
               glm::vec2{0.9f, 0.6f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
               glm::vec4(1, 0, 0, 1)});
    texture_nodes_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);

    memory_usage_ = addComponent<engine::gui::Label>(
               L"GPU memory usage:",
               glm::vec2{0.9f, 0.55f},
               engine::gui::Font{"src/resources/fonts/Vera.ttf", 20,
               glm::vec4(1, 0, 0, 1)});
    memory_usage_->set_horizontal_alignment(Font::HorizontalAlignment::kRight);
  }

  ~FpsDisplay() {
    std::cout << "Average FPS: " << sum_frame_num_ / sum_time_ << std::endl;
  }

 private:
  engine::gui::Label *fps_;
  engine::gui::Label *geom_nodes_, *triangle_count_, *triangle_per_sec_;
  engine::gui::Label *texture_nodes_, *memory_usage_;
  const float kRefreshInterval;
  double sum_frame_num_, sum_time_;

  virtual void update() override {
    static double accum_time = scene_->camera_time().dt;
    static int calls = 0;

    calls++;
    accum_time += scene_->camera_time().dt;
    double fps = calls / accum_time;
    if (accum_time > kRefreshInterval) {
      fps_->set_text(L"FPS: " +
        std::to_wstring(static_cast<int>(fps)));

      size_t geom_nodes_count = engine::GlobalHeightMap::geom_nodes_count;
      geom_nodes_->set_text(L"Geometry nodes: " +
        std::to_wstring(geom_nodes_count));

      size_t triangle_count = (geom_nodes_count
          << (2*engine::GlobalHeightMap::node_dimension_exp)) / 1000;
      triangle_count_->set_text(L"Triangles count: " +
        std::to_wstring(triangle_count) + L"K");

      size_t triangles_per_sec = triangle_count * fps / 1000;
      triangle_per_sec_->set_text(L"Triangles per sec: " +
        std::to_wstring(triangles_per_sec) + L"M");

      texture_nodes_->set_text(L"Texture nodes: " +
        std::to_wstring(engine::GlobalHeightMap::texture_nodes_count));

      size_t gpu_mem_usage = engine::GlobalHeightMap::gpu_mem_usage/1024/1024;
      memory_usage_->set_text(L"GPU memory usage: " +
        std::to_wstring(gpu_mem_usage) + L"MB");

      sum_frame_num_ += calls;
      sum_time_ += accum_time;
      accum_time = calls = 0;
    }
  }
};

#endif
