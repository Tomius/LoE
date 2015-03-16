// Copyright (c) 2015, Tamas Csala

/*   _                    _          __   ____
 *  | |    __ _ _ __   __| |   ___  / _| |  _ \ _ __ ___  __ _ _ __ ___  ___
 *  | |   / _` | '_ \ / _` |  / _ \| |_  | | | | '__/ _ \/ _` | '_ ` _ \/ __|
 *  | |__| (_| | | | | (_| | | (_) |  _| | |_| | | |  __/ (_| | | | | | \__ \
 *  |_____\__,_|_| |_|\__,_|  \___/|_|   |____/|_|  \___|\__,_|_| |_| |_|___/
 */

#include "scenes/main_scene.h"

using engine::GameEngine;

int main(int argc, char* argv[]) {
  try {
    GameEngine::InitContext();
    GameEngine::LoadScene<MainScene>();
    GameEngine::Run();
  } catch(const std::exception& err) {
    std::cerr << err.what();
    GameEngine::Destroy();
    std::terminate();
  }
}
