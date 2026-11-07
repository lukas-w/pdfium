// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_fileaccess_stream.h"

#include <stdint.h>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/fileaccess_iface.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace {

class FakeFileAccess final : public FileAccessIface {
 public:
  FakeFileAccess() = default;
  ~FakeFileAccess() override = default;

  bool Open(const ByteString& file_name) override { return true; }
  void Close() override {}

  FX_FILESIZE GetSize() const override {
    return static_cast<FX_FILESIZE>(buffer_.size());
  }

  FX_FILESIZE GetPosition() const override { return pos_; }

  FX_FILESIZE SetPosition(FX_FILESIZE pos) override {
    if (pos < 0 || static_cast<size_t>(pos) > buffer_.size()) {
      return -1;
    }
    pos_ = pos;
    return pos_;
  }

  size_t Read(pdfium::span<uint8_t> buffer) override {
    if (pos_ < 0 || static_cast<size_t>(pos_) >= buffer_.size()) {
      return 0;
    }
    size_t count =
        std::min(buffer.size(), buffer_.size() - static_cast<size_t>(pos_));
    fxcrt::spancpy(buffer, pdfium::span(buffer_).subspan(
                               static_cast<size_t>(pos_), count));
    pos_ += count;
    return count;
  }

  bool Write(pdfium::span<const uint8_t> buffer) override {
    if (pos_ < 0) {
      return false;
    }
    size_t offset = static_cast<size_t>(pos_);
    if (offset + buffer.size() > buffer_.size()) {
      buffer_.resize(offset + buffer.size());
    }
    fxcrt::spancpy(pdfium::span(buffer_).subspan(offset), buffer);
    pos_ += buffer.size();
    return true;
  }

  size_t ReadPos(pdfium::span<uint8_t> buffer, FX_FILESIZE pos) override {
    if (SetPosition(pos) == -1) {
      return 0;
    }
    return Read(buffer);
  }

  bool Flush() override { return true; }
  bool Truncate(FX_FILESIZE szFile) override { return true; }

 private:
  std::vector<uint8_t> buffer_;
  FX_FILESIZE pos_ = 0;
};

}  // namespace

TEST(CFXFileAccessStreamTest, CreateFromNonExistentFilename) {
  RetainPtr<CFX_FileAccessStream> stream =
      CFX_FileAccessStream::CreateFromFilename("non_existent_file_path.dat");
  EXPECT_FALSE(stream);
}

TEST(CFXFileAccessStreamTest, Operations) {
  auto fake = std::make_unique<FakeFileAccess>();
  auto stream = pdfium::MakeRetain<CFX_FileAccessStream>(std::move(fake));
  ASSERT_TRUE(stream);

  EXPECT_EQ(0, stream->GetSize());
  EXPECT_EQ(0, stream->GetPosition());
  EXPECT_TRUE(stream->IsEOF());
  EXPECT_TRUE(stream->Flush());

  static constexpr uint8_t kData[] = {'a', 'b', 'c', 'd'};
  EXPECT_TRUE(stream->WriteBlock(kData));
  EXPECT_EQ(4, stream->GetSize());
  EXPECT_EQ(4, stream->GetPosition());
  EXPECT_TRUE(stream->IsEOF());

  std::array<uint8_t, 2> read_buf;
  EXPECT_TRUE(stream->ReadBlockAtOffset(read_buf, 1));
  EXPECT_THAT(read_buf, ElementsAre('b', 'c'));
  EXPECT_EQ(3, stream->GetPosition());
  EXPECT_FALSE(stream->IsEOF());

  EXPECT_FALSE(stream->ReadBlockAtOffset(read_buf, 10));
}
