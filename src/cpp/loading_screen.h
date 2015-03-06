// Copyright (c) 2014, Tamas Csala

#ifndef LOD_LOADING_SCREEN_H_
#define LOD_LOADING_SCREEN_H_

#include "engine/oglwrap_config.h"
#include "oglwrap/smart_enums.h"
#include "engine/gui/label.h"

inline void LoadingScreen() {
  glm::vec2 window_size = engine::GameEngine::window_size();
  engine::gui::Label label{
    nullptr, L"Loading please wait...", glm::vec2{0, 0},
    engine::gui::Font{"src/resources/fonts/ObelixPro.ttf", 60}
  };
  label.set_vertical_alignment(engine::gui::Font::VerticalAlignment::kCenter);
  label.set_horizontal_alignment(engine::gui::Font::HorizontalAlignment::kCenter);
  label.set_font_size(64);
  label.set_group(2);
  label.screenResized(window_size.x, window_size.y);

  gl::TemporarySet capabilities{{{gl::kBlend, true},
                                 {gl::kCullFace, false},
                                 {gl::kDepthTest, false}}};
  gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);
  label.render2D();
}

#endif  // LOD_LOADING_SCREEN_H_
