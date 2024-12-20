// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/layout/fgas_linebreak.h"

#include <stddef.h>

#include <array>
#include <iterator>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_unicode.h"

namespace {

#define FX_LBUN FX_LINEBREAKTYPE::kUNKNOWN
#define FX_LBDB FX_LINEBREAKTYPE::kDIRECT_BRK
#define FX_LBIB FX_LINEBREAKTYPE::kINDIRECT_BRK
#define FX_LBCB FX_LINEBREAKTYPE::kCOM_INDIRECT_BRK
#define FX_LBCP FX_LINEBREAKTYPE::kCOM_PROHIBITED_BRK
#define FX_LBPB FX_LINEBREAKTYPE::kPROHIBITED_BRK
#define FX_LBHS FX_LINEBREAKTYPE::kHANGUL_SPACE_BRK

using LineBreakPairRow = std::array<const FX_LINEBREAKTYPE, 38>;
constexpr std::array<const LineBreakPairRow, 38> kLineBreakPairTable = {{
    {FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBPB, FX_LBPB, FX_LBPB, FX_LBCP, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBPB, FX_LBPB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBPB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBPB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBIB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBIB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBIB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBIB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBIB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBDB, FX_LBPB, FX_LBPB, FX_LBPB, FX_LBDB, FX_LBPB,
     FX_LBDB, FX_LBIB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBPB, FX_LBPB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBPB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
    {FX_LBDB, FX_LBPB, FX_LBIB, FX_LBDB, FX_LBIB, FX_LBPB, FX_LBPB, FX_LBPB,
     FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBDB, FX_LBIB, FX_LBIB,
     FX_LBDB, FX_LBDB, FX_LBPB, FX_LBCB, FX_LBPB, FX_LBDB, FX_LBDB, FX_LBDB,
     FX_LBDB, FX_LBDB, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN,
     FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN, FX_LBUN},
}};

#undef FX_LBUN
#undef FX_LBDB
#undef FX_LBIB
#undef FX_LBCB
#undef FX_LBCP
#undef FX_LBPB
#undef FX_LBHS

}  // namespace

FX_LINEBREAKTYPE GetLineBreakTypeFromPair(FX_BREAKPROPERTY curr_char,
                                          FX_BREAKPROPERTY next_char) {
  const size_t row = static_cast<size_t>(curr_char);
  const size_t col = static_cast<size_t>(next_char);
  return kLineBreakPairTable[row][col];
}
