// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_SAMPLEDFUNC_H_
#define CORE_FPDFAPI_PAGE_CPDF_SAMPLEDFUNC_H_

#include <vector>

#include "core/fpdfapi/page/cpdf_function.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_StreamAcc;

class CPDF_SampledFunc final : public CPDF_Function {
 public:
  struct SampleEncodeInfo {
    float encode_max;
    float encode_min;
    uint32_t sizes;
  };

  struct SampleDecodeInfo {
    float decode_max;
    float decode_min;
  };

  CPDF_SampledFunc();
  ~CPDF_SampledFunc() override;

  // CPDF_Function:
  bool v_Init(const CPDF_Object* pObj, VisitedSet* pVisited) override;
  bool v_Call(pdfium::span<const float> inputs,
              pdfium::span<float> results) const override;

  const std::vector<SampleEncodeInfo>& GetEncodeInfo() const {
    return encode_info_;
  }
  uint32_t GetBitsPerSample() const { return bits_per_sample_; }

#if defined(PDF_USE_SKIA)
  RetainPtr<CPDF_StreamAcc> GetSampleStream() const;
#endif

 private:
  std::vector<SampleEncodeInfo> encode_info_;
  std::vector<SampleDecodeInfo> decode_info_;
  uint32_t bits_per_sample_ = 0;
  uint32_t sample_max_ = 0;
  RetainPtr<CPDF_StreamAcc> sample_stream_;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_SAMPLEDFUNC_H_
