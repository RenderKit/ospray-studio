// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"
#include "sg/FileWatcher.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
namespace sg {

Renderer::Renderer(std::string type)
{
  createChild("type", "string", "renderer type", type);
  createChild("pixelSamples", "int", "samples-per-pixel", 1);
  createChild("maxPathLength", "int", "maximum ray recursion depth", 20);
  createChild("minContribution",
      "float",
      "minimum sample contribution, below which value is neglected",
      0.001f);
  createChild("varianceThreshold",
      "float",
      "stop rendering when variance < threshold",
      0.f);
  createChild(
      "backplate_filename", "filename", "Backplate filename", std::string(""));
  createChild("backgroundColor",
      "rgba",
      "transparent background color and alpha (RGBA), if no map_backplate set",
      rgba(vec3f(0.f), 1.f)); // black, with opaque alpha
  createChild("pixelFilter",
      "OSPPixelFilterType",
      "pixel filter used by the renderer for antialiasing\n"
      "(0=point, 1=box, 2=gauss, 3=mitchell, 4=blackman_harris)",
      pixelFilter);

  child("backplate_filename").setSGOnly();
  child("type").setSGNoUI();
  child("type").setSGOnly();

  child("pixelSamples").setMinMax(1, 1000);
  child("maxPathLength").setMinMax(0, 1000);
  child("minContribution").setMinMax(0.f, 10.f);
  child("varianceThreshold").setMinMax(0.f, 100.f);
  child("pixelFilter")
      .setMinMax(OSP_PIXELFILTER_POINT, OSP_PIXELFILTER_BLACKMAN_HARRIS);

  setHandle(cpp::Renderer(child("type").valueAs<std::string>()));
}

NodeType Renderer::type() const
{
  return NodeType::RENDERER;
}

void Renderer::preCommit()
{
  // Finish generic OSPNode precommit
  OSPNode::preCommit();

  // Mark this filename to be watched for asynchronous modification
  // (added here, because the filename node can be added or removed elsewhere)
  addFileWatcher(child("backplate_filename"), "Backplate", true);

  auto filename = child("backplate_filename").valueAs<std::string>();

  // Load backplate file and create texture
  if (filename == "") {
    if (hasChild("map_backplate"))
      remove("map_backplate");
  } else {
    auto mapFilename = hasChild("map_backplate")
        ? child("map_backplate")["filename"].valueAs<std::string>()
        : "";

    // reload or remove if backplate filename changes or file has been modified
    if (child("backplate_filename").isModified() || (filename != mapFilename)) {
      // If map has changed, update backplate filename
      if (hasChild("map_backplate") && child("map_backplate").isModified()) {
        child("backplate_filename") = mapFilename;
        // Remove texture if had a map, but mapFilename has been removed
        if (mapFilename == "")
          remove("map_backplate");
      } else {
        // Otherwise, create/replace map and load backplate filename
        auto &backplateTex = createChild("map_backplate", "texture_2d");
        auto texture = backplateTex.nodeAs<sg::Texture2D>();
        // Force reload rather than using texture cache
        texture->params.reload = (filename == mapFilename);
        if (!texture->load(filename, false, false)) {
          if (hasChild("map_backplate"))
            remove("map_backplate");
          child("backplate_filename") = std::string("");
        }
      }
    }
  }
}

void Renderer::postCommit()
{
  auto asRenderer = valueAs<cpp::Renderer>();

  if (hasChild("map_backplate")) {
    auto &map = child("map_backplate").valueAs<cpp::Texture>();
    asRenderer.setParam("map_backplate", (cpp::Texture)map);
    map.commit();
  } else
    asRenderer.removeParam("map_backplate");

  // Finish node preCommit
  OSPNode::postCommit();
}

// Register OSPRay's debug renderers //
struct OSPSG_INTERFACE DebugRenderer : public Renderer
{
  DebugRenderer();
  virtual ~DebugRenderer() override = default;
};

DebugRenderer::DebugRenderer() : Renderer("debug")
{
  createChild("method", "string", std::string("eyeLight"));
}

OSP_REGISTER_SG_NODE_NAME(DebugRenderer, renderer_debug);

} // namespace sg
} // namespace ospray
