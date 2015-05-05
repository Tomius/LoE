// Copyright (c) 2015, Tamas Csala

#ifndef LOD_LOADING_SCREEN_H_
#define LOD_LOADING_SCREEN_H_

#include "engine/oglwrap_all.h"
#include "engine/gui/label.h"

inline void LoadingScreen() {
  glm::vec2 window_size = engine::GameEngine::window_size();

  engine::gui::Label label{
    nullptr, L"Land of Earth", glm::vec2{0, 0.05},
    engine::gui::Font{"src/resources/fonts/Vera.ttf", 40}
  };
  label.set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
  label.set_horizontal_alignment(engine::gui::Font::HorizontalAlignment::kCenter);
  label.set_group(2);
  label.screenResized(window_size.x, window_size.y);

  engine::gui::Label label2{
    nullptr, L"Loading please wait...", glm::vec2{0, -0.05},
    engine::gui::Font{"src/resources/fonts/ObelixPro.ttf", 18}
  };
  label2.set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
  label2.set_horizontal_alignment(engine::gui::Font::HorizontalAlignment::kCenter);
  label2.set_group(2);
  label2.screenResized(window_size.x, window_size.y);

  gl::TemporarySet capabilities{{{gl::kBlend, true},
                                 {gl::kCullFace, false},
                                 {gl::kDepthTest, false}}};
  gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);
  label.render2D();
  label2.render2D();
}

#endif  // LOD_LOADING_SCREEN_H_
