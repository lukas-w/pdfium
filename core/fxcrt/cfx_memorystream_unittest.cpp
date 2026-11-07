// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_memorystream.h"

#include "core/fxcrt/retain_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXMemoryStreamTest, ReadBlockAtOffset) {
  auto stream = pdfium::MakeRetain<CFX_MemoryStream>();
  const uint8_t kData1[] = {'a', 'b', 'c', 'd', 'e', 'f'};
  ASSERT_TRUE(stream->WriteBlock(kData1));
  ASSERT_THAT(stream->GetSpan(),
              testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f'));

  uint8_t buffer[3];
  ASSERT_TRUE(stream->ReadBlockAtOffset(buffer, 2));
  ASSERT_THAT(buffer, testing::ElementsAre('c', 'd', 'e'));

  ASSERT_TRUE(stream->ReadBlockAtOffset(buffer, 0));
  ASSERT_THAT(buffer, testing::ElementsAre('a', 'b', 'c'));

  ASSERT_TRUE(stream->ReadBlockAtOffset(buffer, 3));
  ASSERT_THAT(buffer, testing::ElementsAre('d', 'e', 'f'));
}

TEST(CFXMemoryStreamTest, WriteBlock) {
  auto stream = pdfium::MakeRetain<CFX_MemoryStream>();
  ASSERT_TRUE(stream->WriteBlock({}));
  ASSERT_TRUE(stream->GetSpan().empty());

  const uint8_t kData1[] = {'a', 'b', 'c'};
  ASSERT_TRUE(stream->WriteBlock(kData1));
  ASSERT_THAT(stream->GetSpan(), testing::ElementsAre('a', 'b', 'c'));

  ASSERT_TRUE(stream->WriteBlock(kData1));
  ASSERT_THAT(stream->GetSpan(),
              testing::ElementsAre('a', 'b', 'c', 'a', 'b', 'c'));

  ASSERT_TRUE(stream->WriteBlock({}));
  ASSERT_THAT(stream->GetSpan(),
              testing::ElementsAre('a', 'b', 'c', 'a', 'b', 'c'));
}

TEST(CFXMemoryStreamTest, WriteZeroBytes) {
  auto stream = pdfium::MakeRetain<CFX_MemoryStream>();
  const uint8_t kData1[] = {'a', 'b', 'c'};
  ASSERT_TRUE(stream->WriteBlock(kData1));
  ASSERT_THAT(stream->GetSpan(), testing::ElementsAre('a', 'b', 'c'));

  ASSERT_TRUE(stream->WriteBlock({}));
  ASSERT_THAT(stream->GetSpan(), testing::ElementsAre('a', 'b', 'c'));
}

TEST(CFXMemoryStreamTest, PositionAndEOF) {
  auto stream = pdfium::MakeRetain<CFX_MemoryStream>();
  EXPECT_EQ(0, stream->GetSize());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_TRUE(stream->IsEOF());
  EXPECT_TRUE(stream->Flush());

  const uint8_t kData[] = {1, 2, 3, 4};
  EXPECT_TRUE(stream->WriteBlock(kData));
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(4, stream->GetPosition());
  EXPECT_TRUE(stream->IsEOF());

  uint8_t buf[2];
  EXPECT_TRUE(stream->ReadBlockAtOffset(buf, 0));
  EXPECT_EQ(2, stream->GetPosition());
  EXPECT_FALSE(stream->IsEOF());

  EXPECT_TRUE(stream->ReadBlockAtOffset(buf, 2));
  EXPECT_EQ(4, stream->GetPosition());
  EXPECT_TRUE(stream->IsEOF());
}

TEST(CFXMemoryStreamTest, ReadBlockAtOffsetInvalid) {
  auto stream = pdfium::MakeRetain<CFX_MemoryStream>();
  const uint8_t kData[] = {1, 2, 3};
  EXPECT_TRUE(stream->WriteBlock(kData));

  // Empty buffer returns false.
  EXPECT_FALSE(stream->ReadBlockAtOffset({}, 0));

  // Negative offset returns false.
  uint8_t buf[1];
  EXPECT_FALSE(stream->ReadBlockAtOffset(buf, -1));

  // Read beyond size returns false.
  EXPECT_FALSE(stream->ReadBlockAtOffset(buf, 3));
  EXPECT_FALSE(stream->ReadBlockAtOffset(buf, 10));
}
