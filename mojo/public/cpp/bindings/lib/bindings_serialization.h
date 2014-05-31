// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_H_

#include <vector>

#include "mojo/public/cpp/bindings/message.h"

namespace mojo {
namespace internal {

class BoundsChecker;

// Please note that this is a different value than |mojo::kInvalidHandleValue|,
// which is the "decoded" invalid handle.
const MojoHandle kEncodedInvalidHandleValue = static_cast<MojoHandle>(-1);

size_t Align(size_t size);
char* AlignPointer(char* ptr);

bool IsAligned(const void* ptr);

// Pointers are encoded as relative offsets. The offsets are relative to the
// address of where the offset value is stored, such that the pointer may be
// recovered with the expression:
//
//   ptr = reinterpret_cast<char*>(offset) + *offset
//
// A null pointer is encoded as an offset value of 0.
//
void EncodePointer(const void* ptr, uint64_t* offset);
const void* DecodePointerRaw(const uint64_t* offset);

template <typename T>
inline void DecodePointer(const uint64_t* offset, T** ptr) {
  *ptr = reinterpret_cast<T*>(const_cast<void*>(DecodePointerRaw(offset)));
}

// Checks whether decoding the pointer will overflow and produce a pointer
// smaller than |offset|.
bool ValidateEncodedPointer(const uint64_t* offset);

// Check that the given pointer references memory contained within the message.
bool ValidatePointer(const void* ptr, const Message& message);

// Handles are encoded as indices into a vector of handles. These functions
// manipulate the value of |handle|, mapping it to and from an index.
void EncodeHandle(Handle* handle, std::vector<Handle>* handles);
bool DecodeHandle(Handle* handle, std::vector<Handle>* handles);

// The following 2 functions are used to encode/decode all objects (structs and
// arrays) in a consistent manner.

template <typename T>
inline void Encode(T* obj, std::vector<Handle>* handles) {
  if (obj->ptr)
    obj->ptr->EncodePointersAndHandles(handles);
  EncodePointer(obj->ptr, &obj->offset);
}

// TODO(yzshen): Remove all redundant validation during decoding. And make
// Decode*() functions/methods return void.
template <typename T>
inline bool Decode(T* obj, Message* message) {
  DecodePointer(&obj->offset, &obj->ptr);
  if (obj->ptr) {
    if (!ValidatePointer(obj->ptr, *message))
      return false;
    if (!obj->ptr->DecodePointersAndHandles(message))
      return false;
  }
  return true;
}

// If returns true, this function also claims the memory range of the size
// specified in the struct header, starting from |data|.
bool ValidateStructHeader(const void* data,
                          uint32_t min_num_bytes,
                          uint32_t min_num_fields,
                          BoundsChecker* bounds_checker);

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_H_
