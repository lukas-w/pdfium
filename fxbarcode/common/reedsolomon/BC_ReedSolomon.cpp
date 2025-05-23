// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2007 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fxbarcode/common/reedsolomon/BC_ReedSolomon.h"

#include <memory>
#include <utility>

#include "fxbarcode/common/reedsolomon/BC_ReedSolomonGF256.h"
#include "fxbarcode/common/reedsolomon/BC_ReedSolomonGF256Poly.h"

CBC_ReedSolomonEncoder::CBC_ReedSolomonEncoder(CBC_ReedSolomonGF256* field)
    : field_(field) {
  cached_generators_.push_back(std::make_unique<CBC_ReedSolomonGF256Poly>(
      field_, std::vector<int32_t>{1}));
}

CBC_ReedSolomonEncoder::~CBC_ReedSolomonEncoder() = default;

CBC_ReedSolomonGF256Poly* CBC_ReedSolomonEncoder::BuildGenerator(
    size_t degree) {
  if (degree >= cached_generators_.size()) {
    CBC_ReedSolomonGF256Poly* lastGenerator = cached_generators_.back().get();
    for (size_t d = cached_generators_.size(); d <= degree; ++d) {
      CBC_ReedSolomonGF256Poly temp_poly(field_, {1, field_->Exp(d - 1)});
      auto nextGenerator = lastGenerator->Multiply(&temp_poly);
      if (!nextGenerator) {
        return nullptr;
      }

      lastGenerator = nextGenerator.get();
      cached_generators_.push_back(std::move(nextGenerator));
    }
  }
  return cached_generators_[degree].get();
}

bool CBC_ReedSolomonEncoder::Encode(std::vector<int32_t>* toEncode,
                                    size_t ecBytes) {
  if (ecBytes == 0) {
    return false;
  }

  if (toEncode->size() <= ecBytes) {
    return false;
  }

  CBC_ReedSolomonGF256Poly* generator = BuildGenerator(ecBytes);
  if (!generator) {
    return false;
  }

  size_t dataBytes = toEncode->size() - ecBytes;
  std::vector<int32_t> infoCoefficients(dataBytes);
  for (size_t x = 0; x < dataBytes; x++) {
    infoCoefficients[x] = (*toEncode)[x];
  }

  CBC_ReedSolomonGF256Poly info(field_, infoCoefficients);
  auto infoTemp = info.MultiplyByMonomial(ecBytes, 1);
  if (!infoTemp) {
    return false;
  }

  auto remainder = infoTemp->Divide(generator);
  if (!remainder) {
    return false;
  }

  const auto& coefficients = remainder->GetCoefficients();
  size_t numZeroCoefficients =
      ecBytes > coefficients.size() ? ecBytes - coefficients.size() : 0;
  for (size_t i = 0; i < numZeroCoefficients; i++) {
    (*toEncode)[dataBytes + i] = 0;
  }
  for (size_t y = 0; y < coefficients.size(); y++) {
    (*toEncode)[dataBytes + numZeroCoefficients + y] = coefficients[y];
  }
  return true;
}
