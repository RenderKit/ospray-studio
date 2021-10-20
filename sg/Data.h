// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Node.h"

namespace ospray {
  namespace sg {

  struct Data : public OSPNode<cpp::CopiedData, NodeType::PARAMETER>
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

    template <typename T, typename ALLOC_T>
    Data(const std::vector<T, ALLOC_T> &arr,
         const vec2ul &numItems,
         bool isShared = false);

    template <typename T, typename ALLOC_T>
    Data(const std::vector<T, ALLOC_T> &arr,
         const vec3ul &numItems,
         bool isShared = false);

    // Set a single object as a 1-item data array

    template <typename T>
    Data(const T &obj);

    vec3ul numItems;
    vec3ul byteStride;
    OSPDataType format;
    bool isShared;

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
  inline Data::Data(const vec3ul &numItems_,
                    const vec3ul &byteStride_,
                    const T *init,
                    bool isShared_)
    : numItems(numItems_)
    , byteStride(byteStride_)
    , isShared(isShared_)
  {
    validate_element_type<T>();

    auto format = OSPTypeFor<T>::value;
    this->format = format;

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

    setValue(cpp::CopiedData(ospObject));
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

  template <typename T, typename ALLOC_T>
  inline Data::Data(const std::vector<T, ALLOC_T> &arr,
                    const vec2ul &numItems,
                    bool isShared)
      : Data(numItems, arr.data(), isShared)
  {
    validate_element_type<T>();
  }

  template <typename T, typename ALLOC_T>
  inline Data::Data(const std::vector<T, ALLOC_T> &arr,
                    const vec3ul &numItems,
                    bool isShared)
      : Data(numItems, arr.data(), isShared)
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
        "box, linear, affine) are expected to come from rkcommon::math.");
  }

  }  // namespace sg
} // namespace ospray
