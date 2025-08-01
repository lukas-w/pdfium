// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jbig2/JBig2_HtrdProc.h"

#include <algorithm>
#include <utility>

#include "core/fxcodec/jbig2/JBig2_BitStream.h"
#include "core/fxcodec/jbig2/JBig2_GrdProc.h"
#include "core/fxcodec/jbig2/JBig2_Image.h"

CJBig2_HTRDProc::CJBig2_HTRDProc() = default;

CJBig2_HTRDProc::~CJBig2_HTRDProc() = default;

std::unique_ptr<CJBig2_Image> CJBig2_HTRDProc::DecodeArith(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts,
    PauseIndicatorIface* pPause) {
  std::unique_ptr<CJBig2_Image> HSKIP;
  if (HENABLESKIP == 1) {
    HSKIP = std::make_unique<CJBig2_Image>(HGW, HGH);
    for (uint32_t mg = 0; mg < HGH; ++mg) {
      for (uint32_t ng = 0; ng < HGW; ++ng) {
        // The `>> 8` is an arithmetic shift per spec.  Cast mg, ng to int,
        // else implicit conversions would evaluate it as unsigned shift.
        // HGX / HGY are 32 bit, HRX / HRY 16, mg / ng 32.
        // The result after >> 8 fits in about 42 bit; int64_t suffices.
        auto mg_int = static_cast<int64_t>(mg);
        auto ng_int = static_cast<int64_t>(ng);
        int64_t x = (HGX + mg_int * HRY + ng_int * HRX) >> 8;
        int64_t y = (HGY + mg_int * HRX - ng_int * HRY) >> 8;

        if ((x + HPW <= 0) | (x >= static_cast<int32_t>(HBW)) | (y + HPH <= 0) |
            (y >= static_cast<int32_t>(HBH))) {
          HSKIP->SetPixel(ng, mg, 1);
        } else {
          HSKIP->SetPixel(ng, mg, 0);
        }
      }
    }
  }
  uint32_t HBPP = 1;
  while (static_cast<uint32_t>(1 << HBPP) < HNUMPATS) {
    ++HBPP;
  }

  CJBig2_GRDProc GRD;
  GRD.MMR = HMMR;
  GRD.GBW = HGW;
  GRD.GBH = HGH;
  GRD.GBTEMPLATE = HTEMPLATE;
  GRD.TPGDON = false;
  GRD.USESKIP = HENABLESKIP;
  GRD.SKIP = HSKIP.get();
  if (HTEMPLATE <= 1) {
    GRD.GBAT[0] = 3;
  } else {
    GRD.GBAT[0] = 2;
  }
  GRD.GBAT[1] = -1;
  if (GRD.GBTEMPLATE == 0) {
    GRD.GBAT[2] = -3;
    GRD.GBAT[3] = -1;
    GRD.GBAT[4] = 2;
    GRD.GBAT[5] = -2;
    GRD.GBAT[6] = -2;
    GRD.GBAT[7] = -2;
  }

  uint8_t GSBPP = static_cast<uint8_t>(HBPP);
  std::vector<std::unique_ptr<CJBig2_Image>> GSPLANES(GSBPP);
  for (int32_t i = GSBPP - 1; i >= 0; --i) {
    std::unique_ptr<CJBig2_Image> pImage;
    CJBig2_GRDProc::ProgressiveArithDecodeState state;
    state.pImage = &pImage;
    state.pArithDecoder = pArithDecoder;
    state.gbContexts = gbContexts;
    state.pPause = nullptr;
    FXCODEC_STATUS status = GRD.StartDecodeArith(&state);
    state.pPause = pPause;
    while (status == FXCODEC_STATUS::kDecodeToBeContinued) {
      status = GRD.ContinueDecode(&state);
    }
    if (!pImage) {
      return nullptr;
    }

    GSPLANES[i] = std::move(pImage);
    if (i < GSBPP - 1) {
      GSPLANES[i]->ComposeFrom(0, 0, GSPLANES[i + 1].get(), JBIG2_COMPOSE_XOR);
    }
  }
  return DecodeImage(GSPLANES);
}

std::unique_ptr<CJBig2_Image> CJBig2_HTRDProc::DecodeMMR(
    CJBig2_BitStream* pStream) {
  uint32_t HBPP = 1;
  while (static_cast<uint32_t>(1 << HBPP) < HNUMPATS) {
    ++HBPP;
  }

  CJBig2_GRDProc GRD;
  GRD.MMR = HMMR;
  GRD.GBW = HGW;
  GRD.GBH = HGH;

  uint8_t GSBPP = static_cast<uint8_t>(HBPP);
  std::vector<std::unique_ptr<CJBig2_Image>> GSPLANES(GSBPP);
  GRD.StartDecodeMMR(&GSPLANES[GSBPP - 1], pStream);
  if (!GSPLANES[GSBPP - 1]) {
    return nullptr;
  }

  pStream->alignByte();
  pStream->addOffset(3);
  for (int32_t J = GSBPP - 2; J >= 0; --J) {
    GRD.StartDecodeMMR(&GSPLANES[J], pStream);
    if (!GSPLANES[J]) {
      return nullptr;
    }

    pStream->alignByte();
    pStream->addOffset(3);
    GSPLANES[J]->ComposeFrom(0, 0, GSPLANES[J + 1].get(), JBIG2_COMPOSE_XOR);
  }
  return DecodeImage(GSPLANES);
}

std::unique_ptr<CJBig2_Image> CJBig2_HTRDProc::DecodeImage(
    const std::vector<std::unique_ptr<CJBig2_Image>>& GSPLANES) {
  auto HTREG = std::make_unique<CJBig2_Image>(HBW, HBH);
  if (!HTREG->data()) {
    return nullptr;
  }

  HTREG->Fill(HDEFPIXEL);
  for (uint32_t mg = 0; mg < HGH; ++mg) {
    for (uint32_t ng = 0; ng < HGW; ++ng) {
      uint32_t gsval = 0;
      for (uint8_t i = 0; i < GSPLANES.size(); ++i) {
        gsval |= GSPLANES[i]->GetPixel(ng, mg) << i;
      }
      uint32_t pat_index = std::min(gsval, HNUMPATS - 1);
      // The `>> 8` is an arithmetic shift per spec.  Cast mg, ng to int,
      // else implicit conversions would evaluate it as unsigned shift.
      // HGX / HGY are 32 bit, HRX / HRY 16, mg / ng 32.
      // The result after >> 8 fits in about 42 bit; int64_t suffices.
      auto mg_int = static_cast<int64_t>(mg);
      auto ng_int = static_cast<int64_t>(ng);
      int64_t x = (HGX + mg_int * HRY + ng_int * HRX) >> 8;
      int64_t y = (HGY + mg_int * HRX - ng_int * HRY) >> 8;
      (*HPATS)[pat_index]->ComposeTo(HTREG.get(), x, y, HCOMBOP);
    }
  }
  return HTREG;
}
