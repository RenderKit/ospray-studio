// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
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

// Althought stb_image.h is in the external directory, there can be only one
// implementation in the entire scene graph.  Implement it here.

#define STB_IMAGE_IMPLEMENTATION // define this in only *one* .cc
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION // define this in only *one* .cc
#include "stb_image_write.h"
