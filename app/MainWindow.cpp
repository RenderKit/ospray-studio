// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MainWindow.h"
#include "MainMenuBuilder.h"
#include <chrono>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "rkcommon/tasking/parallel_for.h"

#include "Proggy.h"

// initialize static member variables
double MainWindow::g_camMoveX = 0.0;
double MainWindow::g_camMoveY = 0.0;
double MainWindow::g_camMoveZ = 0.0;
double MainWindow::g_camMoveA = 0.0;
double MainWindow::g_camMoveE = 0.0;
double MainWindow::g_camMoveR = 0.0;

int MainWindow::g_camPathPause = 2;
int MainWindow::g_rotationConstraint = -1;
double MainWindow::CAM_MOVERATE = 10.0;
bool MainWindow::g_quitNextFrame = false;
bool MainWindow::showUi = true;
bool MainWindow::g_saveNextFrame = false;

// callbacks
void MainWindow::error_callback(int error, const char *desc)
{
  std::cerr << "error " << error << ": " << desc << std::endl;
}

MainWindow::MainWindow(
    const vec2i &_windowSize, std::shared_ptr<GUIContext> _ctx)
    : windowSize(_windowSize), ctx(_ctx)
{
  animationWidget = std::shared_ptr<AnimationWidget>(
      new AnimationWidget("Animation Controls", ctx->animationManager));

  arcballCamera = std::make_shared<ArcballCamera>(
      ctx->frame->child("world").bounds(), windowSize);

  cameraStack = std::make_shared<CameraStack<CameraState>>(arcballCamera);
  // Initialize windows builder and its options
  windowsBuilder = std::make_shared<WindowsBuilder>(ctx, cameraStack);
  // Initialize main menu builder and options
  mainMenuBuilder = std::make_shared<MainMenuBuilder>(ctx, windowsBuilder);
}

// GLFW callbacks
void MainWindow::initGLFW()
{
  glfwSetErrorCallback(error_callback);

  // initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

  // get primary monitor's display scaling
  GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
  if (primaryMonitor)
    glfwGetMonitorContentScale(primaryMonitor, &contentScale.x, &contentScale.y);

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Studio", nullptr, nullptr);
  
  // create a user pointer to our wrapper of GLFWWindow class
  glfwSetWindowUserPointer(glfwWindow, this);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwMakeContextCurrent(glfwWindow);

  // Determine whether GL framebuffer has float format
  // and set format used by glTexImage2D correctly.
  if (glfwGetWindowAttrib(glfwWindow, GLFW_CONTEXT_VERSION_MAJOR) < 3) {
    gl_rgb_format = GL_RGB16;
    gl_rgba_format = GL_RGBA16;
  } else {
    gl_rgb_format = GL_RGB32F;
    gl_rgba_format = GL_RGBA32F;
  }

  glfwSetKeyCallback(
      glfwWindow, [](GLFWwindow *glfwWindow, int key, int scancode, int action, int mod) {
        auto pw = (MainWindow*)glfwGetWindowUserPointer(glfwWindow);
        auto &io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard)
          if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_UP:
              if (mod != GLFW_MOD_ALT)
                g_camMoveZ = -CAM_MOVERATE;
              else
                g_camMoveY = -CAM_MOVERATE;
              break;
            case GLFW_KEY_DOWN:
              if (mod != GLFW_MOD_ALT)
                g_camMoveZ = CAM_MOVERATE;
              else
                g_camMoveY = CAM_MOVERATE;
              break;
            case GLFW_KEY_LEFT:
              g_camMoveX = -CAM_MOVERATE;
              break;
            case GLFW_KEY_RIGHT:
              g_camMoveX = CAM_MOVERATE;
              break;
            case GLFW_KEY_W:
              g_camMoveE = CAM_MOVERATE;
              break;
            case GLFW_KEY_S:
              if (mod != GLFW_MOD_CONTROL)
                g_camMoveE = -CAM_MOVERATE;
              else
                g_saveNextFrame = true;
              break;
            case GLFW_KEY_A:
              if (mod != GLFW_MOD_ALT)
                g_camMoveA = -CAM_MOVERATE;
              else
                g_camMoveR = -CAM_MOVERATE;
              break;
            case GLFW_KEY_D:
              if (mod != GLFW_MOD_ALT)
                g_camMoveA = CAM_MOVERATE;
              else
                g_camMoveR = CAM_MOVERATE;
              break;

            case GLFW_KEY_X:
              g_rotationConstraint = 0;
              break;
            case GLFW_KEY_Y:
              g_rotationConstraint = 1;
              break;
            case GLFW_KEY_Z:
              g_rotationConstraint = 2;
              break;

            case GLFW_KEY_I:
              pw->centerOnEyePos();
              break;

            case GLFW_KEY_G:
              showUi = !showUi;
              break;
            case GLFW_KEY_Q: {
              auto showMode =
                  rkcommon::utility::getEnvVar<int>("OSPSTUDIO_SHOW_MODE");
              // Enforce "ctrl-Q" to make it more difficult to exit by mistake.
              if (showMode && mod != GLFW_MOD_CONTROL)
                std::cout << "Use ctrl-Q to exit\n";
              else
                g_quitNextFrame = true;
            } break;
            case GLFW_KEY_P:
              pw->ctx->printUtilNode("frame");
              break;
            case GLFW_KEY_M:
              pw->ctx->printUtilNode("baseMaterialRegistry");
              break;
            case GLFW_KEY_L:
              pw->ctx->printUtilNode("lightsManager");
              break;
            case GLFW_KEY_B:
              std::cout << pw->ctx->frame->bounds() << std::endl;
              break;
            case GLFW_KEY_V:
              pw->ctx->printUtilNode("camera");
              break;
            case GLFW_KEY_SPACE:
              if (pw->cameraStack->size() >= 2) {
                pw->ctx->g_animatingPath = !pw->ctx->g_animatingPath;
                pw->cameraStack->g_camCurrentPathIndex = 0;
              }
              pw->animationWidget->togglePlay();
              break;
            case GLFW_KEY_EQUAL:
              pw->cameraStack->pushLookMark();
              break;
            case GLFW_KEY_MINUS: {
              // Switch back to default-camera before modifying any parameters
              pw->ctx->changeToDefaultCamera();
              auto valid = pw->cameraStack->popLookMark();
              if (valid)
                pw->ctx->updateCamera();
            } break;
            case GLFW_KEY_0: /* fallthrough */
            case GLFW_KEY_1: /* fallthrough */
            case GLFW_KEY_2: /* fallthrough */
            case GLFW_KEY_3: /* fallthrough */
            case GLFW_KEY_4: /* fallthrough */
            case GLFW_KEY_5: /* fallthrough */
            case GLFW_KEY_6: /* fallthrough */
            case GLFW_KEY_7: /* fallthrough */
            case GLFW_KEY_8: /* fallthrough */
            case GLFW_KEY_9: {
              pw->ctx->changeToDefaultCamera();
              auto valid = pw->cameraStack->setCameraSnapshot(
                  (key + 9 - GLFW_KEY_0) % 10);
              if (valid)
                pw->ctx->updateCamera();
            } break;
            }
          }
        if (action == GLFW_RELEASE) {
          switch (key) {
          case GLFW_KEY_X:
          case GLFW_KEY_Y:
          case GLFW_KEY_Z:
            g_rotationConstraint = -1;
            break;
          case GLFW_KEY_UP:
          case GLFW_KEY_DOWN:
            g_camMoveZ = 0;
            g_camMoveY = 0;
            break;
          case GLFW_KEY_LEFT:
          case GLFW_KEY_RIGHT:
            g_camMoveX = 0;
            break;
          case GLFW_KEY_W:
          case GLFW_KEY_S:
            g_camMoveE = 0;
            break;
          case GLFW_KEY_A:
          case GLFW_KEY_D:
            g_camMoveA = 0;
            g_camMoveR = 0;
            break;
          }
        }
        if (pw->keyCallback)
          pw->keyCallback(pw, key, scancode, action, mod);
      });

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *glfwWindow, int newWidth, int newHeight) {
        auto pw = (MainWindow*)glfwGetWindowUserPointer(glfwWindow);
        pw->windowSize = vec2i{newWidth, newHeight};
        pw->reshape();
      });

  glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow *glfwWindow, int, int action, int) {
    auto pw = (MainWindow*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO &io = ImGui::GetIO();
    if (!showUi || !io.WantCaptureMouse) {
      double x, y;
      glfwGetCursorPos(glfwWindow, &x, &y);
      pw->mouseButton(vec2f{float(x), float(y)});

      pw->ctx->frame->setNavMode(action == GLFW_PRESS);
    }
  });

  glfwSetScrollCallback(glfwWindow, [](GLFWwindow *glfwWindow, double x, double y) {
    auto pw = (MainWindow*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO &io = ImGui::GetIO();
    if (!showUi || !io.WantCaptureMouse) {
      pw->mouseWheel(vec2f{float(x), float(y)});
    }
  });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *glfwWindow, double x, double y) {
    auto pw = (MainWindow*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO &io = ImGui::GetIO();
    if (!showUi || !io.WantCaptureMouse) {
      pw->motion(vec2f{float(x), float(y)});
    }
  });

  ImGui::CreateContext();

  // Enable context for ImGui experimental viewports
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
  ImGui_ImplOpenGL2_Init();

  // Disable active viewports until users enables toggled in view menu
  ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

  // set ImGui font, scaled to display DPI
  auto &io = ImGui::GetIO();
  auto scaleFactor = std::max(contentScale.x, contentScale.y);
  auto scaledFontSize = fontSize * scaleFactor;
  ImVec2 imScale(contentScale.x, contentScale.y);

  ImFont *font = io.Fonts->AddFontFromMemoryCompressedTTF(
      ProggyClean_compressed_data, ProggyClean_compressed_size, scaledFontSize);
  if (!font)
    std::cerr << "ImGui font did not load correctly." << std::endl;
  io.FontGlobalScale = 1.f / scaleFactor;
  io.DisplayFramebufferScale = imScale;

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  ctx->refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &windowSize.x, &windowSize.y);
  reshape();
}

MainWindow::~MainWindow()
{
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}

void MainWindow::mainLoop()
{
  // continue until the user closes the window
  startNewOSPRayFrame();

  while (!glfwWindowShouldClose(glfwWindow) && !g_quitNextFrame) {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool hasPausedRendering = false;
    size_t numTasksExecuted;
    do {
      numTasksExecuted = 0;

      if (ctx->optDoAsyncTasking) {
        numTasksExecuted += ctx->scheduler->background()->executeAllTasksAsync();
      } else {
        numTasksExecuted += ctx->scheduler->background()->executeAllTasksSync();
      }

      numTasksExecuted += ctx->scheduler->studio()->executeAllTasksSync();

      auto task = ctx->scheduler->ospray()->pop();
      if (task) {
        if (!hasPausedRendering) {
          hasPausedRendering = true;

          // if a task wants to modify ospray properties, then we need to cancel any
          // currently rendering frame to make sure we don't get any segfaults
          ctx->frame->pauseRendering = true;
          ctx->frame->cancelFrame();
          ctx->frame->waitOnFrame();
        }

        numTasksExecuted += ctx->scheduler->ospray()->executeAllTasksSync(task);
      }
    } while (numTasksExecuted > 0);

    if (hasPausedRendering) {
      hasPausedRendering = false;

      // after running ospray tasks, make sure to re-enable rendering and update
      // the scene and camera for any newly added or modified objects in the
      // scene
      ctx->frame->pauseRendering = false;
      ctx->refreshScene(true);
    }

    display();

    glfwPollEvents();
  }

  waitOnOSPRayFrame();
}

void MainWindow::reshape(bool reshapeGLFW)
{
  vec2i fSize = windowSize;
  auto frame = ctx->frame;
  if (ctx->lockAspectRatio) {
    // Tell OSPRay to render the largest subset of the window that satisies the
    // aspect ratio
    float aspectCorrection = ctx->lockAspectRatio
        * static_cast<float>(windowSize.y)
        / static_cast<float>(windowSize.x);
    if (aspectCorrection > 1.f) {
      fSize.y /= aspectCorrection;
    } else {
      fSize.x *= aspectCorrection;
    }
  }
  if (frame->child("camera").hasChild("aspect"))
    frame->child("camera")["aspect"] = static_cast<float>(fSize.x) / fSize.y;

  frame->child("windowSize") = fSize;
  frame->currentAccum = 0;

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);
  if (reshapeGLFW)
    glfwSetWindowSize(glfwWindow, windowSize.x, windowSize.y);
}

void MainWindow::resetArcball()
{
  const auto &worldBounds = ctx->frame->child("world").bounds();
  arcballCamera.reset(new ArcballCamera(worldBounds, windowSize));
  cameraStack->updateArcball(arcballCamera);
}

void MainWindow::pickCenterOfRotation(float x, float y)
{
  vec3f worldPosition;
  x = clamp(x / windowSize.x, 0.f, 1.f);
  y = 1.f - clamp(y / windowSize.y, 0.f, 1.f);
  if (ctx->resHasHit(x, y, worldPosition)) {
    if (!(glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)) {
      // Constraining rotation around the up works pretty well.
      arcballCamera->constrainedRotate(vec2f(0.5f, 0.5f), vec2f(x, y), 1);
      // Restore any preFPV zoom level, then clear it.
      arcballCamera->setZoomLevel(preFPVZoom + arcballCamera->getZoomLevel());
      preFPVZoom = 0.f;
      arcballCamera->setCenter(vec3f(worldPosition));
    }
    // Update camera
    ctx->updateCamera();
  }
}

void MainWindow::keyboardMotion()
{
  if (!(g_camMoveX || g_camMoveY || g_camMoveZ || g_camMoveE || g_camMoveA
          || g_camMoveR))
    return;

  // Switch back to default-camera before modifying any parameters
  ctx->changeToDefaultCamera();

  auto sensitivity = maxMoveSpeed;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity *= fineControl;

  // 6 degrees of freedom, four arrow keys? no problem.
  double inOut = g_camMoveZ;
  double leftRight = g_camMoveX;
  double upDown = g_camMoveY;
  double roll = g_camMoveR;
  double elevation = g_camMoveE;
  double azimuth = g_camMoveA;

  if (inOut) {
    arcballCamera->dolly(inOut * sensitivity);
  }
  if (leftRight) {
    arcballCamera->pan(vec2f(leftRight, 0) * sensitivity);
  }
  if (upDown) {
    arcballCamera->pan(vec2f(0, upDown) * sensitivity);
  }
  if (elevation) {
    arcballCamera->constrainedRotate(
        vec2f(-.5, 0), vec2f(-.5, elevation * .005 * sensitivity), 0);
  }
  if (azimuth) {
    arcballCamera->constrainedRotate(
        vec2f(0, -.5), vec2f(azimuth * .005 * sensitivity, -0.5), 1);
  }
  if (roll) {
    arcballCamera->constrainedRotate(
        vec2f(-.5, 0), vec2f(-.5, roll * .005 * sensitivity), 2);
  }
  ctx->updateCamera();
}


void MainWindow::motion(const vec2f &position)
{
  if (ctx->frame->pauseRendering)
    return;

  const vec2f mouse = position * contentScale;
  if (previousMouse == vec2f(-1)) {
    previousMouse = mouse;
    return;
  }

  const bool leftDown =
    glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  const bool rightDown =
    glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  const bool middleDown =
    glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
  const vec2f prev = previousMouse;
  previousMouse = mouse;

  if (!(leftDown || rightDown || middleDown))
    return;

  // Switch back to default-camera before modifying any parameters
  ctx->changeToDefaultCamera();

  auto sensitivity = maxMoveSpeed;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity *= fineControl;

  auto displaySize = windowSize * contentScale;

  const vec2f mouseFrom(clamp(prev.x * 2.f / displaySize.x - 1.f, -1.f, 1.f),
      clamp(prev.y * 2.f / displaySize.y - 1.f, -1.f, 1.f));
  const vec2f mouseTo(clamp(mouse.x * 2.f / displaySize.x - 1.f, -1.f, 1.f),
      clamp(mouse.y * 2.f / displaySize.y - 1.f, -1.f, 1.f));

  if (leftDown) {
    arcballCamera->constrainedRotate(mouseFrom,
        lerp(sensitivity, mouseFrom, mouseTo),
        g_rotationConstraint);
  } else if (rightDown) {
    // empirically, 10.f feels about right.
    // still needs to be scaled based on world bounds
    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      arcballCamera->dolly((mouseTo - mouseFrom).y * 10.f * sensitivity);
    else
      arcballCamera->zoom((mouseTo - mouseFrom).y * 10.f * sensitivity);
  } else if (middleDown) {
    arcballCamera->pan((mouseTo - mouseFrom) * 10.f * sensitivity);
  }

  ctx->updateCamera();
}

void MainWindow::mouseButton(const vec2f &position)
{
  if (ctx->frame->pauseRendering)
    return;
    
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
      && glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    // Switch back to default-camera before modifying any parameters
    ctx->changeToDefaultCamera();

    vec2f scaledPosition = position * contentScale;
    pickCenterOfRotation(scaledPosition.x, scaledPosition.y);
  }
}

void MainWindow::mouseWheel(const vec2f &scroll)
{
  if (!scroll || ctx->frame->pauseRendering)
    return;

  // Switch back to default-camera before modifying any parameters
  ctx->changeToDefaultCamera();

  // scroll is +/- 1 for horizontal/vertical mouse-wheel motion

  auto sensitivity = maxMoveSpeed;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity *= fineControl;

  if (scroll.y)
    ctx->updateFovy(sensitivity, scroll.y);

  // XXX anything interesting to do with horizontal scroll wheel?
  // Perhaps cycle through renderer types?  Or toggle denoiser or navigation?
}

void MainWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (ctx->optAutorotate) {
    vec2f from(0.f, 0.f);
    vec2f to(autorotateSpeed * 0.001f, 0.f);
    arcballCamera->rotate(from, to);
    ctx->updateCamera();
  }

  // Update animation controller if playing
  if (animationWidget->isPlaying()) {
    animationWidget->update();
    ctx->useSceneCamera();
  }

  if (ctx->g_animatingPath) {
    g_camPath = cameraStack->buildPath();

    static int framesPaused = 0;
    auto &g_camCurrentPathIndex = cameraStack->g_camCurrentPathIndex;
    CameraState current = g_camPath[g_camCurrentPathIndex];
    arcballCamera->setState(current);
    ctx->updateCamera();

    // pause at the end of the path
    if (g_camCurrentPathIndex == (int) g_camPath.size() - 1) {
      framesPaused++;
      int framesToWait = g_camPathPause * ImGui::GetIO().Framerate;
      if (framesPaused > framesToWait) {
        framesPaused = 0;
        g_camCurrentPathIndex = 0;
      }
    } else {
      g_camCurrentPathIndex++;
    }
  }

  keyboardMotion();

  if (displayCallback)
    displayCallback(this);

  updateTitleBar();

  fbSize = ctx->getFBSize();

  if (ctx->frame->frameIsReady()) {
    if (!ctx->frame->isCanceled()) {
      // display frame rate in window title
      auto displayEnd = std::chrono::high_resolution_clock::now();
      auto durationMilliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              displayEnd - displayStart);

      latestFPS = 1000.f / float(durationMilliseconds.count());

      // map OSPRay framebuffer, update OpenGL texture with contents, then unmap
      waitOnOSPRayFrame();

      bool displayBufferColor = ctx->optDisplayBuffer & OSP_FB_COLOR;
      bool displayBufferDepth = ctx->optDisplayBuffer & OSP_FB_DEPTH;
      bool displayBufferAccum = ctx->optDisplayBuffer & OSP_FB_ACCUM;
      bool displayBufferVariance = ctx->optDisplayBuffer & OSP_FB_VARIANCE;
      bool displayBufferNormal = ctx->optDisplayBuffer & OSP_FB_NORMAL;
      bool displayBufferAlbedo = ctx->optDisplayBuffer & OSP_FB_ALBEDO;
      bool displayBufferPrimitive = ctx->optDisplayBuffer & OSP_FB_ID_PRIMITIVE;
      bool displayBufferObject = ctx->optDisplayBuffer & OSP_FB_ID_OBJECT;
      bool displayBufferInstance = ctx->optDisplayBuffer & OSP_FB_ID_INSTANCE;

      // optDisplayBufferInvert is separate
      auto *mappedFB = (void *)ctx->frame->mapFrame(
          OSPFrameBufferChannel(ctx->optDisplayBuffer));

      // Only create the copy if it's needed
      std::vector<float> bufferCopy;
      if (ctx->optDisplayBufferInvert
          && (displayBufferColor || displayBufferAlbedo
              || displayBufferNormal)) {
        // Create a local copy and don't modify OSPRay buffer
        const auto *mappedColor = static_cast<const float *>(mappedFB);
        const auto num = displayBufferColor ? 4 : 3;
        std::vector<float> colorCopy(
            mappedColor, mappedColor + fbSize.x * fbSize.y * num);
        bufferCopy = std::move(colorCopy);

      } else if (displayBufferPrimitive
          || displayBufferObject || displayBufferInstance) {
        const auto *mappedID = static_cast<const uint32_t *>(mappedFB);
        std::vector<float> colorsID(fbSize.x * fbSize.y * 3);

        tasking::parallel_for(fbSize.x * fbSize.y, [&](int px) {
          const vec4f color = ospray::sg::makeRandomColor(mappedID[px]);
          colorsID[px * 3 + 0] = color.x;
          colorsID[px * 3 + 1] = color.y;
          colorsID[px * 3 + 2] = color.z;
        });

        bufferCopy = std::move(colorsID);

      } else if (displayBufferDepth) {
        // Create a local copy and don't modify OSPRay buffer
        const auto *mappedDepth = static_cast<const float *>(mappedFB);
        std::vector<float> depthCopy(
            mappedDepth, mappedDepth + fbSize.x * fbSize.y);

        // Scale OSPRay's 0 -> inf depth range to OpenGL 0 -> 1, ignoring all
        // inf values
        float minValue = rkcommon::math::inf;
        float maxValue = rkcommon::math::neg_inf;
        for (auto value : depthCopy) {
          if (isinf(value))
            continue;
          minValue = std::min(minValue, value);
          maxValue = std::max(maxValue, value);
        }

        const float rcpRange = 1.f / (maxValue - minValue);
        std::transform(depthCopy.begin(),
            depthCopy.end(),
            depthCopy.begin(),
            [&](float value) {
              return isinf(value) ? 1.f : (value - minValue) * rcpRange;
            });

        bufferCopy = std::move(depthCopy);
      }

      if (!bufferCopy.empty()) {
        // Inverted values (1.0 -> 0.0) may be more meaningful
        if (ctx->optDisplayBufferInvert)
          std::transform(bufferCopy.begin(),
              bufferCopy.end(),
              bufferCopy.begin(),
              [&](float value) { return (1.f - value); });
      }

      GLenum glType = GL_FLOAT;
      if (displayBufferColor && !ctx->framebuffer->isFloatFormat())
        glType = GL_UNSIGNED_BYTE;

      glBindTexture(GL_TEXTURE_2D, framebufferTexture);
      glTexImage2D(GL_TEXTURE_2D,
          0,
          displayBufferColor ? gl_rgba_format : gl_rgb_format,
          fbSize.x,
          fbSize.y,
          0,
          displayBufferColor       ? GL_RGBA
              : displayBufferDepth ? GL_LUMINANCE
                                   : GL_RGB,
          glType,
          !bufferCopy.empty() ? bufferCopy.data() : mappedFB);

      ctx->frame->unmapFrame(mappedFB);

      // save frame to a file, if requested
      if (g_saveNextFrame) {
        ctx->saveCurrentFrame();
        g_saveNextFrame = false;
      }
    }

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
  }

  // Allow OpenGL to show linear buffers as sRGB.
  if (uiDisplays_sRGB && !ctx->framebuffer->isSRGB())
    glEnable(GL_FRAMEBUFFER_SRGB);

  // clear current OpenGL color buffer
  glClear(GL_COLOR_BUFFER_BIT);
  vec2f border(0.f);

  // clear current OpenGL color buffer
  glClear(GL_COLOR_BUFFER_BIT);

  if (ctx->lockAspectRatio) {
    // when rendered aspect ratio doesn't match window, compute texture
    // coordinates to center the display
    float aspectCorrection = ctx->lockAspectRatio
        * static_cast<float>(windowSize.y) / static_cast<float>(windowSize.x);
    if (aspectCorrection > 1.f) {
      border.y = 1.f - aspectCorrection;
    } else {
      border.x = 1.f - 1.f / aspectCorrection;
    }
  }

  // render textured quad with OSPRay frame buffer contents
  border *= 0.5f;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glBegin(GL_QUADS);

  glTexCoord2f(border.x, border.y);
  glVertex2f(0, 0);

  glTexCoord2f(border.x, 1.f - border.y);
  glVertex2f(0, windowSize.y);

  glTexCoord2f(1.f - border.x, 1.f - border.y);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f - border.x, border.y);
  glVertex2f(windowSize.x, 0);

  glEnd();

  glDisable(GL_FRAMEBUFFER_SRGB);

  if (showUi) {
    // Notify ImGui of the colorspace for color picker widgets
    // (to match the colorspace of the framebuffer)
    if (uiDisplays_sRGB || ctx->framebuffer->isSRGB())
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsSRGB;

    buildUI();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_IsSRGB;
  } else {
    ImGui::EndFrame();
  }

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void MainWindow::startNewOSPRayFrame()
{
  ctx->frame->startNewFrame();
}

void MainWindow::waitOnOSPRayFrame()
{
  ctx->frame->waitOnFrame();
}

void MainWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  ctx->getWindowTitle(windowTitle);
  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}

void MainWindow::centerOnEyePos()
{
  // Switch back to default-camera before modifying any parameters
  ctx->changeToDefaultCamera();

  // Recenters camera at the eye position and zooms all the way in, like FPV
  // Save current zoom level
  preFPVZoom = arcballCamera->getZoomLevel();
  arcballCamera->setCenter(arcballCamera->eyePos());
  arcballCamera->setZoomLevel(0.f);
}

void MainWindow::buildUI()
{
  mainMenuBuilder->start();
  windowsBuilder->start();

  // Add the animation widget's UI
  animationWidget->addUI();

  if (uiCallback)
    uiCallback();

  for (auto &p : ctx->pluginPanels)
    p->buildUI(ImGui::GetCurrentContext());
}

void MainWindow::setLockUpDir(const vec3f &lockUpDir)
{
  arcballCamera->setLockUpDir(lockUpDir);
}

void MainWindow::setUpDir(const vec3f &upDir)
{
  arcballCamera->setUpDir(upDir);
}

void MainWindow::animationSetShowUI()
{
  animationWidget->setShowUI();
}

void MainWindow::quitNextFrame() {
  g_quitNextFrame = true;
}
