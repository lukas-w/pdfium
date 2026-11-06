// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/icc/icc_transform.h"

#include <stdint.h>

#include <algorithm>
#include <array>
#include <memory>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/ptr_util.h"
#include "core/fxcrt/span_util.h"
#include "core/fxcrt/zip.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

namespace {

// For use with std::unique_ptr<cmsHPROFILE>.
struct CmsProfileDeleter {
  inline void operator()(cmsHPROFILE p) { cmsCloseProfile(p); }
};

using ScopedCmsProfile = std::unique_ptr<void, CmsProfileDeleter>;

bool Check3Components(cmsColorSpaceSignature cs) {
  switch (cs) {
    case cmsSigGrayData:
    case cmsSigCmykData:
      return false;
    default:
      return true;
  }
}

}  // namespace

IccTransform::IccTransform(cmsHTRANSFORM hTransform,
                           size_t srcComponents,
                           bool bIsLab,
                           bool bNormal)
    : transform_(hTransform),
      src_components_(srcComponents),
      lab_(bIsLab),
      normal_(bNormal) {}

IccTransform::~IccTransform() {
  cmsDeleteTransform(transform_);
}

// static
std::unique_ptr<IccTransform> IccTransform::CreateTransformSRGB(
    pdfium::span<const uint8_t> span) {
  ScopedCmsProfile srcProfile(cmsOpenProfileFromMem(
      span.data(), pdfium::checked_cast<cmsUInt32Number>(span.size())));
  if (!srcProfile) {
    return nullptr;
  }

  ScopedCmsProfile dstProfile(cmsCreate_sRGBProfile());
  if (!dstProfile) {
    return nullptr;
  }

  cmsColorSpaceSignature srcCS = cmsGetColorSpace(srcProfile.get());
  uint32_t nSrcComponents = cmsChannelsOf(srcCS);

  if (!IsValidIccComponents(nSrcComponents)) {
    return nullptr;
  }

  int srcFormat;
  bool bLab = false;
  bool bNormal = false;
  if (srcCS == cmsSigLabData) {
    srcFormat = FLOAT_SH(1) | COLORSPACE_SH(PT_Lab) |
                CHANNELS_SH(nSrcComponents) | BYTES_SH(4);
    bLab = true;
  } else {
    srcFormat =
        COLORSPACE_SH(PT_ANY) | CHANNELS_SH(nSrcComponents) | BYTES_SH(1);
    // TODO(thestig): Check to see if lcms2 supports more colorspaces that can
    // be considered normal.
    bNormal = srcCS == cmsSigGrayData || srcCS == cmsSigRgbData ||
              srcCS == cmsSigCmykData;
  }
  cmsColorSpaceSignature dstCS = cmsGetColorSpace(dstProfile.get());
  if (!Check3Components(dstCS)) {
    return nullptr;
  }

  cmsHTRANSFORM hTransform = nullptr;
  switch (dstCS) {
    case cmsSigRgbData:
      hTransform =
          cmsCreateTransform(srcProfile.get(), srcFormat, dstProfile.get(),
                             TYPE_BGR_8, INTENT_PERCEPTUAL, /*dwFlags=*/0);
      break;
    case cmsSigGrayData:
    case cmsSigCmykData:
      // Check3Components() already filtered these types.
      NOTREACHED();
    default:
      break;
  }
  if (!hTransform) {
    return nullptr;
  }

  // Private ctor.
  return pdfium::WrapUnique(
      new IccTransform(hTransform, nSrcComponents, bLab, bNormal));
}

void IccTransform::Translate(pdfium::span<const float> src_values,
                             pdfium::span<float, 3> dest_values) {
  CHECK_GE(src_values.size(), src_components_);
  constexpr size_t kMaxValidComponents = 4u;  // See IsValidIccComponents().
  uint8_t output[kMaxValidComponents];
  if (lab_) {
    cmsDoTransform(transform_, src_values.data(), output, 1);
  } else {
    std::array<uint8_t, kMaxValidComponents> inputs;
    for (auto [src, dst] :
         fxcrt::Zip(src_values.first(src_components_), inputs)) {
      dst = static_cast<uint8_t>(std::clamp(src * 255.0f, 0.0f, 255.0f));
    }
    cmsDoTransform(transform_, inputs.data(), output, 1);
  }
  dest_values[0] = output[2] / 255.0f;
  dest_values[1] = output[1] / 255.0f;
  dest_values[2] = output[0] / 255.0f;
}

void IccTransform::TranslateScanline(pdfium::span<uint8_t> dest,
                                     pdfium::span<const uint8_t> src,
                                     int32_t pixels) {
  CHECK_GT(pixels, 0);
  const size_t pixel_count = static_cast<size_t>(pixels);
  CHECK_GE(src.size(), pixel_count * src_components_);
  CHECK_GE(dest.size(), pixel_count * 3);
  const cmsUInt32Number format = cmsGetTransformInputFormat(transform_);
  if (lab_) {
    CHECK_EQ(T_FLOAT(format), 1u);
    CHECK_EQ(T_BYTES(format), 4u);
    CHECK_EQ(T_CHANNELS(format), 3u);

    // LittleCMS lacks an input format for 8-bit PDF Lab encoding. TYPE_LabV2_8
    // expects legacy ICC v2 integer encoding, which distorts neutral gray
    // (a*=0, b*=0) and scales L*=100 to 100.392. Promote 8-bit PDF Lab samples
    // to float inputs (TYPE_Lab_FLT) to preserve accurate colors.
    auto lab_span =
        fxcrt::reinterpret_span<const FX_LAB_STRUCT<uint8_t>>(src).first(
            pixel_count);
    auto inputs =
        FixedSizeDataVector<FX_LAB_STRUCT<float>>::Uninit(pixel_count);
    for (auto [src_lab, dst_lab] : fxcrt::Zip(lab_span, inputs.span())) {
      dst_lab = DecodeLab(src_lab);
    }
    cmsDoTransform(transform_, inputs.span().data(), dest.data(), pixels);
  } else {
    CHECK_EQ(T_FLOAT(format), 0u);
    CHECK_EQ(T_BYTES(format), 1u);
    CHECK_EQ(T_CHANNELS(format), static_cast<uint32_t>(src_components_));
    cmsDoTransform(transform_, src.data(), dest.data(), pixels);
  }
}

// static
bool IccTransform::IsValidIccComponents(int components) {
  // According to PDF spec, number of components must be 1, 3, or 4.
  return components == 1 || components == 3 || components == 4;
}

}  // namespace fxcodec
