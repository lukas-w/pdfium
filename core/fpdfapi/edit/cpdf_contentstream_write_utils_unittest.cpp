// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_contentstream_write_utils.h"

#include <math.h>

#include <limits>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string_wrappers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

ByteString WriteFloatToString(float value) {
  fxcrt::ostringstream stream;
  WriteFloat(stream, value);
  return ByteString(stream);
}

}  // namespace

TEST(CPDFContentstreamWriteUtils, WriteFloat) {
  EXPECT_EQ("0", WriteFloatToString(0.0f));
  EXPECT_EQ("0", WriteFloatToString(-0.0f));
  EXPECT_EQ("1", WriteFloatToString(1.0f));
  EXPECT_EQ("-1", WriteFloatToString(-1.0f));
  EXPECT_EQ(".5", WriteFloatToString(0.5f));
  EXPECT_EQ("-.5", WriteFloatToString(-0.5f));
  EXPECT_EQ(".00125", WriteFloatToString(0.00125f));
  EXPECT_EQ("123.45", WriteFloatToString(123.45f));
  EXPECT_EQ("-7.5", WriteFloatToString(-7.5f));
  EXPECT_EQ("38.895287", WriteFloatToString(38.895285f));
  EXPECT_EQ("-77.03723", WriteFloatToString(-77.037232f));
  EXPECT_EQ("340282350000000000000000000000000000000",
            WriteFloatToString(std::numeric_limits<float>::max()));
  EXPECT_EQ("-340282350000000000000000000000000000000",
            WriteFloatToString(-std::numeric_limits<float>::max()));
  EXPECT_EQ(".000000000000000000000000000000000000011754944",
            WriteFloatToString(std::numeric_limits<float>::min()));
  EXPECT_EQ("-.000000000000000000000000000000000000011754944",
            WriteFloatToString(-std::numeric_limits<float>::min()));
  EXPECT_EQ("340282350000000000000000000000000000000",
            WriteFloatToString(INFINITY));
  EXPECT_EQ("-340282350000000000000000000000000000000",
            WriteFloatToString(-INFINITY));
  EXPECT_EQ("0", WriteFloatToString(NAN));
}

TEST(CPDFContentstreamWriteUtils, WriteMatrix) {
  fxcrt::ostringstream stream;
  WriteMatrix(stream, CFX_Matrix(1.0f, 0.0f, 0.0f, 1.0f, 10.5f, 20.25f));
  EXPECT_EQ("1 0 0 1 10.5 20.25", ByteString(stream));
}

TEST(CPDFContentstreamWriteUtils, WritePoint) {
  fxcrt::ostringstream stream;
  WritePoint(stream, CFX_PointF(1.0f, 2.5f));
  EXPECT_EQ("1 2.5", ByteString(stream));
}

TEST(CPDFContentstreamWriteUtils, WriteRect) {
  fxcrt::ostringstream stream;
  WriteRect(stream, CFX_FloatRect(1.0f, 2.0f, 10.0f, 20.0f));
  EXPECT_EQ("1 2 9 18", ByteString(stream));
}
