// ======================================================================== //
// Copyright 2020 Intel Corporation                                    //
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

#include "Generator.h"
// std
#include <random>
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "sg/texture/TextureVolume.h"

namespace ospray::sg {

  struct TextureVolumeTest : public Generator
  {
    TextureVolumeTest();
    ~TextureVolumeTest() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(TextureVolumeTest, generator_texture_volume_test);

  // TextureVolumeTest definitions ////////////////////////////////////////////////

  TextureVolumeTest::TextureVolumeTest()
  {
  }

  void TextureVolumeTest::generateData()
  {
    int size = 256;
    std::vector<float> volumetricData(size * size * size, 0);

    const float r    = 30;
    const float R    = 80;
    const int size_2 = size / 2;

    for (int x = 0; x < size; ++x) {
      for (int y = 0; y < size; ++y) {
        for (int z = 0; z < size; ++z) {
          const float X = x - size_2;
          const float Y = y - size_2;
          const float Z = z - size_2;

          const float d = (R - std::sqrt(X * X + Y * Y));
          volumetricData[size * size * x + size * y + z] =
              r * r - d * d - Z * Z;
        }
      }
    }

    // create transfer func and volume to fetch textureVol from
    auto &texVolume = createChild("map_kd", "textureVolume");
    auto &tf =
        texVolume.createChild("transferFunction", "transfer_function_jet");
    tf.createChild("valueRange", "vec2f", vec2f(-10000.f, 10000.f));

    std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
                                 vec3f(0.0f, 1.0f, 0.0f)};
    tf.createChildData("color", colors);                            
    auto &volume         = tf.createChild("volume", "volume_structured");
    volume["voxelType"]  = int(OSP_FLOAT);
    volume["gridOrigin"] = vec3f(-0.5f, -0.5f, -0.5f);
    volume["gridSpacing"] = vec3f(1.f / size, 1.f / size, 1.f / size);
    volume["dimensions"]  = vec3i(size);
    volume.createChildData("data", vec3i(size), 0, volumetricData.data());

        // create sphere geometry
    auto &spheres = texVolume.createChild("geometry", "geometry_spheres");
    vec3f center  = vec3f{1.f, 1.f, 1.f};
    float radius  = 1.f;
    spheres.createChildData("sphere.position", center);
    spheres.child("radius") = radius;

  }

}  // namespace ospray::sg