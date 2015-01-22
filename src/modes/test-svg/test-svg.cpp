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

static void unpackRGB(uint32_t color, float &r, float &g, float &b) {
  r = (color         & 0xff) / 255.0f;
  g = ((color >> 8)  & 0xff) / 255.0f;
  b = ((color >> 16) & 0xff) / 255.0f;
}

static VGColorRampSpreadMode fromNSVG(NSVGspreadType spread) {
  switch (spread) {
    case NSVG_SPREAD_REFLECT: return VG_COLOR_RAMP_SPREAD_REFLECT;
    case NSVG_SPREAD_REPEAT: return VG_COLOR_RAMP_SPREAD_REPEAT;
    case NSVG_SPREAD_PAD:
    default:
      return VG_COLOR_RAMP_SPREAD_PAD;
  }
}

static VGJoinStyle fromNSVG(NSVGlineJoin join) {
  switch (join) {
    case NSVG_JOIN_ROUND: return VG_JOIN_ROUND;
    case NSVG_JOIN_BEVEL: return VG_JOIN_BEVEL;
    case NSVG_JOIN_MITER:
    default:
      return VG_JOIN_MITER;
  }
}

static VGCapStyle fromNSVG(NSVGlineCap cap) {
  switch (cap) {
    case NSVG_CAP_ROUND: return VG_CAP_ROUND;
    case NSVG_CAP_SQUARE: return VG_CAP_SQUARE;
    case NSVG_CAP_BUTT:
    default:
      return VG_CAP_BUTT;
  }
}

static VGPaint createPaintFromNSVGpaint(const NSVGpaint &svgPaint, float opacity = 1.0f) {
  auto paint = vgCreatePaint();

  if (svgPaint.type == NSVG_PAINT_COLOR) {
    VGfloat color[4];
    unpackRGB(svgPaint.color, color[0], color[1], color[2]);
    color[3] = opacity;
    vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);
  }

  // TODO(ryan): We don't need gradients yet, but I got about halfway through
  // implementing them. Finish this up when we actually need them.

  // else if (svgPaint.type == NSVG_PAINT_LINEAR_GRADIENT ||
  //          svgPaint.type == NSVG_PAINT_RADIAL_GRADIENT) {
  //   const auto &grad = *svgPaint.gradient;

  //   if (svgPaint.type == NSVG_PAINT_LINEAR_GRADIENT) {
  //     VGfloat points[] = {
  //     };
  //     vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
  //     vgSetParameterfv(paint, VG_PAINT_LINEAR_GRADIENT, 4, points);
  //   }

  //   VGfloat stops[5 * grad.nstops];
  //   for (int i = 0; i < grad.nstops; ++i) {
  //     auto s = &stops[i * 5];
  //     s[0] = grad.stops[i].offset;
  //     unpackRGB(grad.stops[i].color, s[1], s[2], s[3]);
  //     s[4] = 1.0f; // TODO(ryan): Are NSVG gradients always opaque?
  //   }

  //   vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, fromNSVG(static_cast<NSVGspreadType>(grad.spread)));
  //   vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_PREMULTIPLIED, false);
  //   vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, 5 * grad.nstops, stops);
  // }

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
      strokeJoin(fromNSVG(static_cast<NSVGlineJoin>(shape->strokeLineJoin)));
      strokeCap(fromNSVG(static_cast<NSVGlineCap>(shape->strokeLineCap)));
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

    vgDrawPath(vgPath, (hasFill   ? VG_FILL_PATH   : 0) |
                       (hasStroke ? VG_STROKE_PATH : 0));
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
