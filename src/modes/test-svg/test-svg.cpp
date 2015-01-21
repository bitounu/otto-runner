#include "nanosvg.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <VG/openvg.h>
#include <iostream>

extern "C" {
  int init();
  int shutdown();
  int update();
  int rotary_changed(int delta);
}

static float r = 0.0f;

int init() {
  std::cout << "Hello" << std::endl;
  return 0;
}

int shutdown() {
  return 0;
}

int update() {
  VGfloat color[4] = { r, 0, 0, 1 };
  vgSetfv(VG_CLEAR_COLOR, 4, color);
  vgClear(0, 0, 96, 96);

  return 0;
}

int rotary_changed(int delta) {
  r += delta * 0.05f;
  std::cout << r << " : " << delta << std::endl;

  return 0;
}
