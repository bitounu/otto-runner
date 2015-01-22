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
  return 0;
}

int shutdown() {
  return 0;
}

static VGPaint createPaintFromNSVGpaint(const NSVGpaint &svgPaint, float opacity = 1.0f) {
  auto paint = vgCreatePaint();

  if (svgPaint.type == NSVG_PAINT_COLOR) {
    float color[] = {
      (svgPaint.color         & 0xff) / 255.0f,
      ((svgPaint.color >> 8)  & 0xff) / 255.0f,
      ((svgPaint.color >> 16) & 0xff) / 255.0f,
      opacity
    };
    vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);
  }

  return paint;
}

static void strokePaint(const NSVGpaint &svgPaint, float opacity = 1.0f) {
  auto paint = createPaintFromNSVGpaint(svgPaint, opacity);
  vgSetPaint(paint, VG_STROKE_PATH);
  vgDestroyPaint(paint);
}

static void fillPaint(const NSVGpaint &svgPaint, float opacity = 1.0f) {
  auto paint = createPaintFromNSVGpaint(svgPaint, opacity);
  vgSetPaint(paint, VG_FILL_PATH);
  vgDestroyPaint(paint);
}

static void strokeColor(float r, float g, float b, float a = 1.0f) {
  VGfloat color[] = { r, g, b, a };
  VGPaint paint = vgCreatePaint();
  vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
  vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);
  vgSetPaint(paint, VG_STROKE_PATH);
  vgDestroyPaint(paint);
}

static void strokeWidth(VGfloat width) {
  vgSetf(VG_STROKE_LINE_WIDTH, width);
}

static void strokeCap(VGCapStyle cap) {
  vgSeti(VG_STROKE_CAP_STYLE, cap);
}

static void strokeJoin(VGJoinStyle join) {
  vgSeti(VG_STROKE_JOIN_STYLE, join);
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

static void draw(const NSVGimage &svg) {
  for (auto shape = svg.shapes; shape != NULL; shape = shape->next) {
    bool hasStroke = shape->stroke.type != NSVG_PAINT_NONE;
    bool hasFill = shape->fill.type != NSVG_PAINT_NONE;

    if (hasFill) {
      fillPaint(shape->fill, shape->opacity);
    }

    if (hasStroke) {
      strokeWidth(shape->strokeWidth);
      strokeJoin(static_cast<VGJoinStyle>(shape->strokeLineJoin));
      strokeCap(static_cast<VGCapStyle>(shape->strokeLineCap));
      strokePaint(shape->stroke, shape->opacity);
    }

    auto vgPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);

    for (auto path = shape->paths; path != NULL; path = path->next) {
      moveTo(vgPath, path->pts[0], path->pts[1]);
      for (int i = 0; i < path->npts - 1; i += 3) {
        float* p = &path->pts[i * 2];
        cubicTo(vgPath, p[2], p[3], p[4], p[5], p[6], p[7]);
      }
    }

    vgDrawPath(vgPath, VG_FILL_PATH | VG_STROKE_PATH);
    vgDestroyPath(vgPath);
  }
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

  draw(*image);

  return 0;
}

int rotary_changed(int delta) {
  rotation += delta * 5.0f;
  return 0;
}
