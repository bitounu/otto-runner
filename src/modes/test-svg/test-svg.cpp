#include <VG/openvg.h>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <math.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

extern "C" {
  int init();
  int shutdown();
  int update();
  int rotary_changed(int delta);
}

static float rotation = 0.0f;

static NSVGimage* image = nullptr;

int init() {
  image = nsvgParseFromFile("src/modes/test-svg/test.svg", "px", 96);
  printf("size: %f x %f\n", image->width, image->height);
  return 0;
}

int shutdown() {
  return 0;
}

static void strokeColor(float r, float g, float b, float a = 1.0f) {
  VGfloat color[] = { r, g, b, a };
  VGPaint strokePaint = vgCreatePaint();
  vgSetParameteri(strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
  vgSetParameterfv(strokePaint, VG_PAINT_COLOR, 4, color);
  vgSetPaint(strokePaint, VG_STROKE_PATH);
  vgDestroyPaint(strokePaint);
}

static void strokeWidth(VGfloat width) {
  vgSetf(VG_STROKE_LINE_WIDTH, width);
  vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
  vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
}

static void moveTo(VGPath path, float x, float y) {
  VGubyte segs[]   = { VG_MOVE_TO_ABS };
  VGfloat coords[] = { x, y };
  vgAppendPathData(path, 1, segs, coords);
}

static void cubicTo(VGPath path, float x1, float y1, float x2, float y2, float x3, float y3) {
  VGubyte segs[]   = { VG_CUBIC_TO };
  VGfloat coords[] = { x1, y1, x2, y2, x3, y3 };
  vgAppendPathData(path, 1, segs, coords);
}

int update() {
  static const float screenWidth  = 96.0f;
  static const float screenHeight = 96.0f;
  static const float defaultMatrix[] =
    { -0.0f, -1.0f, -0.0f, -1.0f,  0.0f,  0.0f, screenWidth, screenHeight, 1.0f };

  static const VGfloat bgColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

  vgSetfv(VG_CLEAR_COLOR, 4, bgColor);
  vgClear(0, 0, 96, 96);

  vgLoadMatrix(defaultMatrix);
  vgTranslate(48, 48);
  vgRotate(rotation);
  vgTranslate(-48, -48);

  strokeColor(1.0f, 0.0f, 0.0f);
  strokeWidth(2.0f);

  for (auto shape = image->shapes; shape != NULL; shape = shape->next) {
    for (auto path = shape->paths; path != NULL; path = path->next) {
      auto vgPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);

      for (int i = 0; i < path->npts - 1; i += 3) {
        float* p = &path->pts[i * 2];

        moveTo(vgPath, p[0], p[1]);
        cubicTo(vgPath, p[2], p[3], p[4], p[5], p[6], p[7]);
      }

      vgDrawPath(vgPath, VG_STROKE_PATH);
      vgDestroyPath(vgPath);
    }
  }

  return 0;
}

int rotary_changed(int delta) {
  rotation += delta * 2.0f;
  return 0;
}
