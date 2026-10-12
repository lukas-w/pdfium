// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_BYTESTRING_POOL_H_
#define CORE_FXCRT_BYTESTRING_POOL_H_

#include "core/fxcrt/bytestring.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace fxcrt {

class ByteStringPool {
 public:
  ByteStringPool();
  ~ByteStringPool();

  ByteString Intern(const ByteString& str);

 private:
  absl::flat_hash_set<ByteString> pool_;
};

}  // namespace fxcrt

using fxcrt::ByteStringPool;

#endif  // CORE_FXCRT_BYTESTRING_POOL_H_
