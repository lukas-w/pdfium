// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/bytestring_pool.h"

#include <memory>

#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace fxcrt {

namespace {

class ByteStringPoolImpl final : public ByteStringPool {
 public:
  ByteStringPoolImpl() = default;
  ~ByteStringPoolImpl() override = default;

  ByteString Intern(const ByteString& str) override {
    if (str.IsEmpty()) {
      return str;
    }
    return *pool_.insert(str).first;
  }

 private:
  absl::flat_hash_set<ByteString> pool_;
};

}  // namespace

// static
std::unique_ptr<ByteStringPool> ByteStringPool::Create() {
  return std::make_unique<ByteStringPoolImpl>();
}

}  // namespace fxcrt
