
// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#pragma once
#include "cereal/archives/xml.hpp"
#include "cereal/types/vector.hpp"
#include "imgui.h"
#include "widgets/TransferFunctionWidget.h"

using namespace rkcommon::math;

namespace ospray {
  namespace sg {
  
struct VolumeParameters
{
  float sigmaTScale = 1.f;
  float sigmaSScale = 1.f;
  bool useLogScale  = true;
};

struct LightParameters
{
  float ambientLightIntensity           = 1.f;
  float directionalLightIntensity       = 1.f;
  float directionalLightAngularDiameter = 45.f;
  vec3f directionalLightDirection       = vec3f(0.f, 0.f, 1.f);
  float sunSkyAlbedo                    = 0.18f;
  float sunSkyTurbidity = 3.f;
  float sunSkyIntensity = 1.f;
  vec3f sunSkyColor = vec3f(1.f);
  vec3f sunSkyUp    = vec3f(0.f, 1.f, 0.f);
  vec3f sunSkyDirection = vec3f(0.f, 0.f, 1.f);
};

struct PathtracerParameters
{
  int lightSamples                    = 1;
  int roulettePathLength                = 5.f;
  float maxContribution                 = 0.f;
};

  struct State
  {
    State();
    ~State();

    // void saveState(const std::string &filename)
    // {
    //   std::ofstream ofs(filename);

    //   if (!ofs.good()) {
    //     std::cerr << "unable to open output file: " << filename << std::endl;
    //     return;
    //   }

    //   {
    //     cereal::XMLOutputArchive oarchive(ofs);

    //     oarchive(g_volumeParameters);
    //     oarchive(g_pathtracerParameters);

    //     for (auto &tfw : g_transferFunctionWidgets) {
    //       tfw->saveArchive(oarchive);
    //     }
    //   }

    //   std::cout << "saved state to: " << filename << std::endl;
    // }

    // void loadState(const std::string &filename)
    // {
    //   std::ifstream ifs;
    //   ifs.open(filename, std::ifstream::in);

    //   if (!ifs.good()) {
    //     std::cerr << "unable to open input file: " << filename << std::endl;
    //     return;
    //   }

    //   {
    //     cereal::XMLInputArchive iarchive(ifs);

    //     iarchive(g_volumeParameters);
    //     iarchive(g_pathtracerParameters);

    //     for (auto &tfw : g_transferFunctionWidgets) {
    //       tfw->loadArchive(iarchive);
    //     }
    //   }

    //   std::cout << "loaded state from: " << filename << std::endl;

    //   if (std::find(g_stateFilenames.begin(),
    //                 g_stateFilenames.end(),
    //                 filename) == g_stateFilenames.end()) {
    //     g_stateFilenames.push_back(filename);
    //   }
    // }

    // bool addSaveLoadStateUI()
    // {
    //   bool changed = false;

    //   static std::array<char, 512> filenameInput{'\0'};

    //   ImGui::InputText(
    //       "state filename", filenameInput.data(), filenameInput.size() - 1);

    //   if (ImGui::Button("Save")) {
    //     saveState(std::string(filenameInput.data()));
    //   }

    //   ImGui::SameLine();

    //   if (ImGui::Button("Load")) {
    //     loadState(std::string(filenameInput.data()));
    //     changed = true;
    //   }

    //   ImGui::SameLine();

    //   if (ImGui::Button("Reset camera")) {
    //     // if (GLFWOSPRayWindow::getActiveWindow()) {
    //     //   GLFWOSPRayWindow::getActiveWindow()->resetCamera();
    //     }
    // //   }

    //   ImGui::Spacing();
    //   ImGui::Separator();
    //   ImGui::Spacing();

    //   return changed;
    // }

    // bool addStatePresetsUI()
    // {
    //   bool changed = false;

    //   ImGui::Begin("State presets");

    //   static int currentStateIndex = 0;

    //   std::vector<const char *> listItems;

    //   for (int i = 0; i < g_stateFilenames.size(); i++) {
    //     listItems.push_back(g_stateFilenames[i].c_str());
    //   }

    //   if (listItems.size() > 0) {
    //     if (ImGui::ListBox("Filenames",
    //                        &currentStateIndex,
    //                        listItems.data(),
    //                        listItems.size())) {
    //       loadState(g_stateFilenames[currentStateIndex]);
    //       changed = true;
    //     }
    //   }

    //   ImGui::End();

    //   return changed;
    // }

    std::vector<VolumeParameters> g_volumeParameters;
    PathtracerParameters g_pathtracerParameters;
    std::vector<std::string> g_stateFilenames;

    std::vector<std::shared_ptr<TransferFunctionWidget>>
        g_transferFunctionWidgets;
  };

}  // namespace sg
} // namespace ospray
