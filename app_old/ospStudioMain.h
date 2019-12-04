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

#include "ospray/sg/SceneGraph.h"
#include "ospray/sg/geometry/TriangleMesh.h"
#include "ospray/sg/visitor/GatherNodesByName.h"
#include "ospray/sg/visitor/MarkAllAsModified.h"

#include "sg_utility/utility.h"

// custom visitors
#include "sg_visitors/GetVoxelRangeOfAllVolumes.h"
#include "sg_visitors/RecomputeBounds.h"
#include "sg_visitors/ReplaceAllTFs.h"

#include "widgets/MainWindow.h"

#include <ospray/ospcommon/utility/StringManip.h>
#include "common/RenderContext.h"
#include "ospcommon/FileName.h"
#include "ospray/sg/texture/Texture2D.h"
#include "ospcommon/utility/getEnvVar.h"

template <class T>
class CmdLineParam
{
 public:
  CmdLineParam(T defaultValue) { value = defaultValue; }
  T getValue() { return value; }

  CmdLineParam &operator=(const CmdLineParam &other)
  {
    value = other.value;
    overridden = true;
    return *this;
  }

  bool isOverridden() { return overridden; }

 private:
  T value;
  bool overridden = false;
};
