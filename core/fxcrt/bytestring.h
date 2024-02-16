// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_BYTESTRING_H_
#define CORE_FXCRT_BYTESTRING_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <functional>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <utility>

#include "core/fxcrt/fx_string_wrappers.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/string_data_template.h"
#include "core/fxcrt/string_template.h"
#include "core/fxcrt/string_view_template.h"
#include "third_party/base/check.h"
#include "third_party/base/containers/span.h"

namespace fxcrt {

// A mutable string with shared buffers using copy-on-write semantics that
// avoids the cost of std::string's iterator stability guarantees.
class ByteString : public StringTemplate<char> {
 public:
  [[nodiscard]] static ByteString FormatInteger(int i);
  [[nodiscard]] static ByteString FormatFloat(float f);
  [[nodiscard]] static ByteString Format(const char* pFormat, ...);
  [[nodiscard]] static ByteString FormatV(const char* pFormat, va_list argList);

  ByteString() = default;
  ByteString(const ByteString& other) = default;

  // Move-construct a ByteString. After construction, |other| is empty.
  ByteString(ByteString&& other) noexcept = default;

  ~ByteString() = default;

  // Make a one-character string from a char.
  explicit ByteString(char ch);

  // Deliberately implicit to avoid calling on every string literal.
  // NOLINTNEXTLINE(runtime/explicit)
  ByteString(const char* ptr);

  // No implicit conversions from wide strings.
  // NOLINTNEXTLINE(runtime/explicit)
  ByteString(wchar_t) = delete;

  ByteString(const char* pStr, size_t len);
  ByteString(const uint8_t* pStr, size_t len);

  explicit ByteString(ByteStringView bstrc);
  ByteString(ByteStringView str1, ByteStringView str2);
  ByteString(const std::initializer_list<ByteStringView>& list);
  explicit ByteString(const fxcrt::ostringstream& outStream);

  // Explicit conversion to C-style string. The result is never nullptr,
  // and is always NUL terminated.
  // Note: Any subsequent modification of |this| will invalidate the result.
  const char* c_str() const { return m_pData ? m_pData->m_String : ""; }

  size_t GetStringLength() const {
    return m_pData ? strlen(m_pData->m_String) : 0;
  }

  int Compare(ByteStringView str) const;
  bool EqualNoCase(ByteStringView str) const;

  bool operator==(const char* ptr) const;
  bool operator==(ByteStringView str) const;
  bool operator==(const ByteString& other) const;

  bool operator!=(const char* ptr) const { return !(*this == ptr); }
  bool operator!=(ByteStringView str) const { return !(*this == str); }
  bool operator!=(const ByteString& other) const { return !(*this == other); }

  bool operator<(const char* ptr) const;
  bool operator<(ByteStringView str) const;
  bool operator<(const ByteString& other) const;

  ByteString& operator=(const char* str);
  ByteString& operator=(ByteStringView str);
  ByteString& operator=(const ByteString& that);

  // Move-assign a ByteString. After assignment, |that| is empty.
  ByteString& operator=(ByteString&& that) noexcept;

  ByteString& operator+=(char ch);
  ByteString& operator+=(const char* str);
  ByteString& operator+=(const ByteString& str);
  ByteString& operator+=(ByteStringView str);

  void SetAt(size_t index, char c);

  void Reserve(size_t len);

  ByteString Substr(size_t offset) const;
  ByteString Substr(size_t first, size_t count) const;
  ByteString First(size_t count) const;
  ByteString Last(size_t count) const;

  void MakeLower();
  void MakeUpper();

  void Trim();
  void Trim(char target);
  void Trim(ByteStringView targets);

  void TrimLeft();
  void TrimLeft(char target);
  void TrimLeft(ByteStringView targets);

  void TrimRight();
  void TrimRight(char target);
  void TrimRight(ByteStringView targets);

  size_t Replace(ByteStringView pOld, ByteStringView pNew);

  uint32_t GetID() const { return AsStringView().GetID(); }

 protected:
  intptr_t ReferenceCountForTesting() const;

  friend class ByteString_Assign_Test;
  friend class ByteString_Concat_Test;
  friend class ByteString_Construct_Test;
  friend class StringPool_ByteString_Test;
};

inline bool operator==(const char* lhs, const ByteString& rhs) {
  return rhs == lhs;
}
inline bool operator==(ByteStringView lhs, const ByteString& rhs) {
  return rhs == lhs;
}
inline bool operator!=(const char* lhs, const ByteString& rhs) {
  return rhs != lhs;
}
inline bool operator!=(ByteStringView lhs, const ByteString& rhs) {
  return rhs != lhs;
}
inline bool operator<(const char* lhs, const ByteString& rhs) {
  return rhs.Compare(lhs) > 0;
}
inline bool operator<(const ByteStringView& lhs, const ByteString& rhs) {
  return rhs.Compare(lhs) > 0;
}
inline bool operator<(const ByteStringView& lhs, const char* rhs) {
  return lhs < ByteStringView(rhs);
}

inline ByteString operator+(ByteStringView str1, ByteStringView str2) {
  return ByteString(str1, str2);
}
inline ByteString operator+(ByteStringView str1, const char* str2) {
  return ByteString(str1, str2);
}
inline ByteString operator+(const char* str1, ByteStringView str2) {
  return ByteString(str1, str2);
}
inline ByteString operator+(ByteStringView str1, char ch) {
  return ByteString(str1, ByteStringView(ch));
}
inline ByteString operator+(char ch, ByteStringView str2) {
  return ByteString(ByteStringView(ch), str2);
}
inline ByteString operator+(const ByteString& str1, const ByteString& str2) {
  return ByteString(str1.AsStringView(), str2.AsStringView());
}
inline ByteString operator+(const ByteString& str1, char ch) {
  return ByteString(str1.AsStringView(), ByteStringView(ch));
}
inline ByteString operator+(char ch, const ByteString& str2) {
  return ByteString(ByteStringView(ch), str2.AsStringView());
}
inline ByteString operator+(const ByteString& str1, const char* str2) {
  return ByteString(str1.AsStringView(), str2);
}
inline ByteString operator+(const char* str1, const ByteString& str2) {
  return ByteString(str1, str2.AsStringView());
}
inline ByteString operator+(const ByteString& str1, ByteStringView str2) {
  return ByteString(str1.AsStringView(), str2);
}
inline ByteString operator+(ByteStringView str1, const ByteString& str2) {
  return ByteString(str1, str2.AsStringView());
}

std::ostream& operator<<(std::ostream& os, const ByteString& str);
std::ostream& operator<<(std::ostream& os, ByteStringView str);

// This is declared here for use in gtest-based tests but is defined in a test
// support target. This should not be used in production code. Just use
// operator<< from above instead.
// In some cases, gtest will automatically use operator<< as well, but in this
// case, it needs PrintTo() because ByteString looks like a container to gtest.
void PrintTo(const ByteString& str, std::ostream* os);

}  // namespace fxcrt

using ByteString = fxcrt::ByteString;

uint32_t FX_HashCode_GetA(ByteStringView str);
uint32_t FX_HashCode_GetLoweredA(ByteStringView str);
uint32_t FX_HashCode_GetAsIfW(ByteStringView str);
uint32_t FX_HashCode_GetLoweredAsIfW(ByteStringView str);

namespace std {

template <>
struct hash<ByteString> {
  size_t operator()(const ByteString& str) const {
    return FX_HashCode_GetA(str.AsStringView());
  }
};

}  // namespace std

extern template struct std::hash<ByteString>;

#endif  // CORE_FXCRT_BYTESTRING_H_
