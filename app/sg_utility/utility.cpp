// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "utility.h"

// ospray_sg
#include "sg/generator/Generator.h"

#include "../sg_visitors/RecomputeBounds.h"

namespace ospray {
  namespace sg {

    void createDefaultView(const sg::Frame &root)
    {
      auto &world = root["renderer"]["world"];
      auto bbox   = world.bounds();
      vec3f diag  = bbox.size();
      diag        = max(diag, vec3f(0.3f * length(diag)));

      auto gaze = ospcommon::center(bbox);
      auto pos =
          gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
      auto up = vec3f(0.f, 1.f, 0.f);

      auto &camera  = root["camera"];
      camera["pos"] = pos;
      camera["dir"] = normalize(gaze - pos);
      camera["up"]  = up;

      camera["focusDistance"] = length(gaze - pos);
    }

    std::tuple<vec3f, vec3f, vec3f> getViewFromCamera(const sg::Frame &root)
    {
      auto &camera = root["camera"];

      auto pos  = camera["pos"].valueAs<vec3f>();
      auto dir  = camera["dir"].valueAs<vec3f>();
      auto up   = camera["up"].valueAs<vec3f>();
      auto dist = camera["focusDistance"].valueAs<float>();

      auto gaze = pos + (normalize(dir) * dist);

      return std::make_tuple(pos, gaze, up);
    }

    std::shared_ptr<sg::Node> createImporterNode(
        const std::string &fileToImport)
    {
      FileName fn = fileToImport;
      std::stringstream ss;
      ss << fn.name();
      auto importerNode_ptr = sg::createNode(ss.str(), "Importer");
      auto &importerNode    = *importerNode_ptr;

      importerNode["fileName"] = fn.str();

      if (importerNode.hasChildRecursive("gradientShadingEnabled"))
        importerNode.childRecursive("gradientShadingEnabled") = false;
      if (importerNode.hasChildRecursive("samplingRate"))
        importerNode.childRecursive("samplingRate") = 0.25f;
      if (importerNode.hasChildRecursive("adaptiveMaxSamplingRate"))
        importerNode.childRecursive("adaptiveMaxSamplingRate") = 1.0f;

      auto transform = createNode("transform_" + ss.str(), "Transform");
      transform->add(importerNode_ptr);

      importerNode.traverse(sg::RecomputeBounds{});
      importerNode.verify();

      return transform;
    }

    std::shared_ptr<sg::Node> createGeneratorNode(const std::string &type,
                                                  const std::string &parameters)
    {
      auto transformNode_ptr = sg::createNode("Transform_" + type, "Transform");

      auto generatorNode_ptr = sg::createNode("Generator_" + type, "Generator")
                                   ->nodeAs<sg::Generator>();
      transformNode_ptr->add(generatorNode_ptr);
      auto &generatorNode = *generatorNode_ptr;

      generatorNode["generatorType"] = type;
      generatorNode["parameters"]    = parameters;

      generatorNode.generateData();

      return transformNode_ptr;
    }

  }  // namespace sg
}  // namespace ospray
