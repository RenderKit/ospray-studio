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

#include "Node.h"

namespace ospray {
  namespace sg {

    struct Data : public OSPNode<cpp::Data, NodeType::PARAMETER>
    {
      Data()           = default;
      ~Data() override = default;

      template <typename T>
      Data(size_t numItems, const T *init, bool isShared = false);

      template <typename T>
      Data(size_t numItems,
           size_t byteStride,
           const T *init,
           bool isShared = false);

      template <typename T>
      Data(const vec2ul &numItems, const T *init, bool isShared = false);

      template <typename T>
      Data(const vec2ul &numItems,
           const vec2ul &byteStride,
           const T *init,
           bool isShared = false);

      template <typename T>
      Data(const vec3ul &numItems, const T *init, bool isShared = false);

      template <typename T>
      Data(const vec3ul &numItems,
           const vec3ul &byteStride,
           const T *init,
           bool isShared = false);

      // Adapters to work with existing STL containers

      template <typename T, std::size_t N>
      Data(const std::array<T, N> &arr, bool isShared = false);

      template <typename T, typename ALLOC_T>
      Data(const std::vector<T, ALLOC_T> &arr, bool isShared = false);

      // Set a single object as a 1-item data array

      template <typename T>
      Data(const T &obj);

     private:
      template <typename T>
      void validate_element_type();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename T>
    inline Data::Data(size_t numItems, const T *init, bool isShared)
        : Data(numItems, 0, init, isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(size_t numItems,
                      size_t byteStride,
                      const T *init,
                      bool isShared)
        : Data(vec3ul(numItems, 1, 1), vec3ul(byteStride, 0, 0), init, isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(const vec2ul &numItems, const T *init, bool isShared)
        : Data(numItems, vec2ul(0), init, isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(const vec2ul &numItems,
                      const vec2ul &byteStride,
                      const T *init,
                      bool isShared)
        : Data(vec3ul(numItems.x, numItems.y, 1),
               vec3ul(byteStride.x, byteStride.y, 0),
               init,
               isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(const vec3ul &numItems, const T *init, bool isShared)
        : Data(numItems, vec3ul(0), init, isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(const vec3ul &numItems,
                      const vec3ul &byteStride,
                      const T *init,
                      bool isShared)
    {
      validate_element_type<T>();

      auto format = OSPTypeFor<T>::value;

      auto tmp = ospNewSharedData(init,
                                  format,
                                  numItems.x,
                                  byteStride.x,
                                  numItems.y,
                                  byteStride.y,
                                  numItems.z,
                                  byteStride.z);

      auto ospObject = tmp;

      if (!isShared) {
        ospObject = ospNewData(format, numItems.x, numItems.y, numItems.z);
        ospCopyData(tmp, ospObject);
        ospRelease(tmp);
      }

      setValue(cpp::Data(ospObject));
    }

    template <typename T, std::size_t N>
    inline Data::Data(const std::array<T, N> &arr, bool isShared)
        : Data(N, arr.data(), isShared)
    {
      validate_element_type<T>();
    }

    template <typename T, typename ALLOC_T>
    inline Data::Data(const std::vector<T, ALLOC_T> &arr, bool isShared)
        : Data(arr.size(), arr.data(), isShared)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline Data::Data(const T &obj) : Data(1, &obj)
    {
      validate_element_type<T>();
    }

    template <typename T>
    inline void Data::validate_element_type()
    {
      static_assert(
          OSPTypeFor<T>::value != OSP_UNKNOWN,
          "Only types corresponding to OSPDataType values can be set "
          "as elements in OSPRay Data arrays. NOTE: Math types (vec, "
          "box, linear, affine) are expected to come from ospcommon::math.");
    }

  }  // namespace sg
}  // namespace ospray
