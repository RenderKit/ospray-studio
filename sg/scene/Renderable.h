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

#include "sg/Node.h"

namespace ospray {
  namespace sg {

    template <typename HANDLE_T>
    struct OSPSG_INTERFACE Renderable : public OSPNode<HANDLE_T>
    {
      Renderable();
      virtual ~Renderable() override = default;

      virtual void preRender();
      virtual void postRender();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename HANDLE_T>
    inline Renderable<HANDLE_T>::Renderable()
    {
      Node::createChild("bounds",
                        "box3f",
                        "Bounding box of this node and its children",
                        box3f());
    }

    template <typename HANDLE_T>
    inline void Renderable<HANDLE_T>::preRender()
    {
    }

    template <typename HANDLE_T>
    inline void Renderable<HANDLE_T>::postRender()
    {
    }

  }  // namespace sg
}  // namespace ospray
