// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_BYTEORDER_H_
#define CORE_FXCRT_BYTEORDER_H_

#include "build/build_config.h"
#include "third_party/base/containers/span.h"
#include "third_party/base/sys_byteorder.h"

namespace fxcrt {

// NOTE: Prefer *Swap*() methods when data is known to be aligned.

// Converts the bytes in |x| from host order (endianness) to little endian, and
// returns the result.
inline uint16_t ByteSwapToLE16(uint16_t x) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return x;
#else
  return pdfium::base::ByteSwap(x);
#endif
}

inline uint32_t ByteSwapToLE32(uint32_t x) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return x;
#else
  return pdfium::base::ByteSwap(x);
#endif
}

// Converts the bytes in |x| from host order (endianness) to big endian, and
// returns the result.
inline uint16_t ByteSwapToBE16(uint16_t x) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return pdfium::base::ByteSwap(x);
#else
  return x;
#endif
}

inline uint32_t ByteSwapToBE32(uint32_t x) {
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  return pdfium::base::ByteSwap(x);
#else
  return x;
#endif
}

// NOTE: These methods exist to improve readability by putting the word "From"
// into the name, otherwise it is less clear that `x = ByteSwapToLE16(y)` gives
// `x` in the native representation when `y` is in a LE representation.
inline uint16_t FromLE16(uint16_t x) {
  return ByteSwapToLE16(x);
}
inline uint32_t FromLE32(uint16_t x) {
  return ByteSwapToLE32(x);
}
inline uint16_t FromBE16(uint16_t x) {
  return ByteSwapToBE16(x);
}
inline uint32_t FromBE32(uint16_t x) {
  return ByteSwapToBE32(x);
}

// Transfer to/from spans irrespective of alignments.
inline uint16_t GetUInt16MSBFirst(pdfium::span<const uint8_t> span) {
  return (static_cast<uint32_t>(span[0]) << 8) | static_cast<uint32_t>(span[1]);
}

inline uint32_t GetUInt32MSBFirst(pdfium::span<const uint8_t> span) {
  return (static_cast<uint32_t>(span[0]) << 24) |
         (static_cast<uint32_t>(span[1]) << 16) |
         (static_cast<uint32_t>(span[2]) << 8) | static_cast<uint32_t>(span[3]);
}

inline uint16_t GetUInt16LSBFirst(pdfium::span<const uint8_t> span) {
  return (static_cast<uint32_t>(span[1]) << 8) | static_cast<uint32_t>(span[0]);
}

inline uint32_t GetUInt32LSBFirst(pdfium::span<const uint8_t> span) {
  return (static_cast<uint32_t>(span[3]) << 24) |
         (static_cast<uint32_t>(span[2]) << 16) |
         (static_cast<uint32_t>(span[1]) << 8) | static_cast<uint32_t>(span[0]);
}

inline void PutUInt16MSBFirst(uint16_t value, pdfium::span<uint8_t> span) {
  span[0] = value >> 8;
  span[1] = value & 0xff;
}

inline void PutUInt32MSBFirst(uint32_t value, pdfium::span<uint8_t> span) {
  span[0] = value >> 24;
  span[1] = (value >> 16) & 0xff;
  span[2] = (value >> 8) & 0xff;
  span[3] = value & 0xff;
}

inline void PutUInt16LSBFirst(uint16_t value, pdfium::span<uint8_t> span) {
  span[1] = value >> 8;
  span[0] = value & 0xff;
}

inline void PutUInt32LSBFirst(uint32_t value, pdfium::span<uint8_t> span) {
  span[3] = value >> 24;
  span[2] = (value >> 16) & 0xff;
  span[1] = (value >> 8) & 0xff;
  span[0] = value & 0xff;
}

}  // namespace fxcrt

#endif  // CORE_FXCRT_BYTEORDER_H_
