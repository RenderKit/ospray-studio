// The MIT License (MIT)
// Copyright (c) 2019 Will Usher
// https://github.com/Twinklebear/ChameleonRT

#pragma once

#include "gltf_types.h"

// TODO extend and use rkcommon::ArrayView instead

// GLTF Buffer view/accessor utilities

struct BufferView {
    const uint8_t *buf = nullptr;
    size_t length = 0;
    size_t stride = 0;

    BufferView(const tinygltf::BufferView &view, const tinygltf::Model &model, size_t base_stride);

    BufferView() = default;

    // Return the pointer to some element, based on the stride specified for the view
    const uint8_t *operator[](const size_t i) const;
};

template <typename T>
class Accessor {
    BufferView view;
    size_t count = 0;

public:
    Accessor(const tinygltf::Accessor &accessor, const tinygltf::Model &model);

    Accessor() = default;

    const T &operator[](const size_t i) const;

    size_t size() const;
    const T *data() const;
    size_t byteStride() const;
};

// implementation

BufferView::BufferView(const tinygltf::BufferView &view,
    const tinygltf::Model &model,
    size_t base_stride)
    : buf(model.buffers[view.buffer].data.data() + view.byteOffset),
      length(view.byteLength),
      stride(std::max(view.byteStride, base_stride))
{}

inline const uint8_t *BufferView::operator[](const size_t i) const
{
  return buf + i * stride;
}

template <typename T>
Accessor<T>::Accessor(const tinygltf::Accessor &accessor, const tinygltf::Model &model)
    : view(model.bufferViews[accessor.bufferView],
           model,
           gltf_base_stride(accessor.type, accessor.componentType)),
      count(accessor.count)
{
    // Apply the additional accessor-specific byte offset
    view.buf += accessor.byteOffset;
}

template <typename T>
const T &Accessor<T>::operator[](const size_t i) const
{
    return *reinterpret_cast<const T *>(view[i]);
}

template <typename T>
size_t Accessor<T>::size() const
{
    return count;
}

template <typename T>
const T *Accessor<T>::data() const
{
  return reinterpret_cast<const T *>(view.buf);
}

template <typename T>
size_t Accessor<T>::byteStride() const
{
  return view.stride;
}
