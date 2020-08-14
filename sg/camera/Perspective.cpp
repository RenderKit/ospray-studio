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

#include "Camera.h"

namespace ospray::sg {

  struct Perspective : public Camera
  {
    Perspective();
  };

  Perspective::Perspective() : Camera("perspective")
  {
    createChild("fovy", "float", "Field-of-view in degrees", 60.f);
    child("fovy").setMinMax(0.f, 180.f);
    createChild("aspect", "float", 1.f);
    child("aspect").setReadOnly();
    createChild("interpupillaryDistance",
                "float",
                "Distance between left and right eye for stereo mode",
                0.0635f);
    child("interpupillaryDistance").setMinMax(0.f, 0.1f);
    createChild("stereoMode",
                "int",
                "0=none, 1=left, 2=right, 3=side-by-side, 4=top-bottom",
                0);
  }

  OSP_REGISTER_SG_NODE_NAME(Perspective, camera_perspective);

}  // namespace ospray::sg
