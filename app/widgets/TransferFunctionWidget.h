// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <ospray/ospray.h>
#include "rkcommon/math/vec.h"
#include "../../sg/scene/transfer_function/TransferFunction.h"

using namespace rkcommon::math;

using ColorPoint   = vec4f;
using OpacityPoint = vec2f;

class TransferFunctionWidget
{
 public:
  TransferFunctionWidget(std::shared_ptr<ospray::sg::TransferFunction> transferFunction,
                         std::function<void()> transferFunctionUpdatedCallback,
                         const vec2f &valueRange       = vec2f(-1.f, 1.f),
                         const std::string &widgetName = "Transfer Function");
  ~TransferFunctionWidget();

  // update UI and process any UI events
  void updateUI();

  void loadDefaultMaps();
  void setMap(int);
  void drawEditor();

  vec3f interpolateColor(const std::vector<ColorPoint> &controlPoints, float x);
  float interpolateOpacity(const std::vector<OpacityPoint> &controlPoints,
                           float x);

  // transfer function being modified and associated callback whenver it's
  // updated
  std::shared_ptr<ospray::sg::TransferFunction> transferFunction{nullptr};
  std::function<void()> transferFunctionUpdatedCallback{nullptr};

    // domain (value range) of transfer function
  vec2f valueRange{-1.f, 1.f};

  // widget name (use different names to support multiple concurrent widgets)
  std::string widgetName;

  // called to perform actual transfer function updates when control points
  // change in UI
  std::function<void(const std::vector<ColorPoint> &,
                     const std::vector<OpacityPoint> &)>
      updateTransferFunction{nullptr};

  void updateTfnPaletteTexture();

  // number of samples for interpolated transfer function passed to OSPRay
  int numSamples{256};

  // all available transfer functions
  std::vector<std::string> tfnsNames;
  std::vector<std::vector<ColorPoint>> tfnsColorPoints;
  std::vector<std::vector<OpacityPoint>> tfnsOpacityPoints;
  std::vector<bool> tfnsEditable;

  // flag indicating transfer function has changed in UI
  bool tfnChanged{true};

  // properties of currently selected transfer function
  int currentMap{0};
  std::vector<ColorPoint> *tfnColorPoints;
  std::vector<OpacityPoint> *tfnOpacityPoints;
  bool tfnEditable{true};

  // texture for displaying transfer function color palette
  GLuint tfnPaletteTexture{0};

  // scaling factor for generated opacities passed to OSPRay
  float globalOpacityScale{1.f};

  // input dialog for save / load filename
  std::array<char, 512> filenameInput{{'\0'}};
};
