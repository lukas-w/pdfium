// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/fuzzers/pdf_jpx_fuzzer_common.h"

#include <memory>

#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fpdfapi/page/jpx_decode_conversion.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"

namespace {

constexpr uint32_t kMaxJPXFuzzSize = 100 * 1024 * 1024;  // 100 MB

bool CheckImageSize(const CJPX_Decoder::JpxImageInfo& image_info) {
  static constexpr uint32_t kMemLimitBytes = 1024 * 1024 * 1024;  // 1 GB.
  FX_SAFE_UINT32 mem = image_info.width;
  mem *= image_info.height;
  mem *= image_info.channels;
  return mem.IsValid() && mem.ValueOrDie() <= kMemLimitBytes;
}

}  // namespace

void DecodeJpxForFuzzing(pdfium::span<const uint8_t> data,
                         CJPX_Decoder::ColorSpaceOption color_space_option,
                         uint8_t resolution_levels_to_skip,
                         bool strict_mode) {
  std::unique_ptr<CJPX_Decoder> decoder = CJPX_Decoder::Create(
      data, color_space_option, resolution_levels_to_skip, strict_mode);
  if (!decoder) {
    return;
  }

  // A call to StartDecode could be too expensive if image size is very big, so
  // check size before calling StartDecode().
  CJPX_Decoder::JpxImageInfo image_info = decoder->GetInfo();
  if (!CheckImageSize(image_info)) {
    return;
  }

  if (!decoder->StartDecode()) {
    return;
  }

  // StartDecode() could change image size, so check again.
  image_info = decoder->GetInfo();
  if (!CheckImageSize(image_info)) {
    return;
  }

  // TODO(thestig): Add colorspace support.
  RetainPtr<CPDF_ColorSpace> color_space;
  auto maybe_conversion =
      JpxDecodeConversion::Create(image_info, color_space.Get());
  if (!maybe_conversion.has_value()) {
    return;
  }

  const auto& conversion = maybe_conversion.value();
  int components = conversion.jpx_components_count().value_or(0);
  if (components <= 0) {
    return;
  }

  image_info.width = conversion.width();

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!bitmap->Create(image_info.width, image_info.height,
                      conversion.format())) {
    return;
  }

  if (bitmap->GetHeight() <= 0 ||
      kMaxJPXFuzzSize / bitmap->GetPitch() <
          static_cast<uint32_t>(bitmap->GetHeight())) {
    return;
  }

  decoder->Decode(bitmap->GetWritableBuffer(), bitmap->GetPitch(),
                  conversion.swap_rgb(), components);
}
