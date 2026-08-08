// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_GIF_CFX_GIFCONTEXT_H_
#define CORE_FXCODEC_GIF_CFX_GIFCONTEXT_H_

#include <memory>
#include <vector>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/gif/cfx_gif.h"
#include "core/fxcodec/gif/lzw_decompressor.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_CodecMemory;
class CFX_DIBitmap;

namespace fxcodec {

class ProgressiveDecoderContextDelegate;

class CFX_GifContext : public ProgressiveDecoderContext {
 public:
  explicit CFX_GifContext(ProgressiveDecoderContextDelegate* delegate);
  ~CFX_GifContext() override;

  void ReadScanline(int32_t row_num, pdfium::span<uint8_t> row_buf);
  bool GetRecordPosition(uint32_t cur_pos,
                         int32_t sub_left,
                         int32_t sub_top,
                         int32_t sub_width,
                         pdfium::span<CFX_GifPalette> pal,
                         int32_t trans_index);
  ProgressiveDecoderContext::Status ReadHeader(int* width, int* height);
  ProgressiveDecoderContext::Status ReadHeader();
  ProgressiveDecoderContext::Status GetFrame();
  ProgressiveDecoderContext::Status LoadFrame(size_t frame_num);

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;
  Status DecodeImage() override;

  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);
  FXCODEC_STATUS ContinueDecode();

  void SetInputBuffer(RetainPtr<CFX_CodecMemory> codec_memory);
  size_t GetFrameNum() const { return images_.size(); }

  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_DIBitmap> bitmap_;
  std::vector<CFX_GifPalette> global_palette_;
  uint8_t global_palette_exp_ = 0;
  uint32_t img_row_offset_ = 0;
  uint32_t img_row_avail_size_ = 0;
  GifDecoderStatus decode_status_ = GifDecoderStatus::kSig;
  std::unique_ptr<CFX_GifGraphicControlExtension> graphic_control_extension_;
  std::vector<std::unique_ptr<CFX_GifImage>> images_;
  std::unique_ptr<LZWDecompressor> lzw_decompressor_;
  int width_ = 0;
  int height_ = 0;
  int img_left_ = 0;
  int img_top_ = 0;
  int img_width_ = 0;
  int trans_index_ = -1;
  DataVector<uint8_t> scanline_buf_;
  uint8_t bc_index_ = 0;
  uint8_t global_sort_flag_ = 0;
  uint8_t global_color_resolution_ = 0;
  uint8_t img_pass_num_ = 0;

 protected:
  bool ReadAllOrNone(pdfium::span<uint8_t> dest);
  ProgressiveDecoderContext::Status ReadGifSignature();
  ProgressiveDecoderContext::Status ReadLogicalScreenDescriptor();

  RetainPtr<CFX_CodecMemory> input_buffer_;

 private:
  void SaveDecodingStatus(GifDecoderStatus status);
  ProgressiveDecoderContext::Status DecodeExtension();
  ProgressiveDecoderContext::Status DecodeImageInfo();
  void DecodingFailureAtTailCleanup(CFX_GifImage* gif_image);
  bool ScanForTerminalMarker();
  uint8_t GetPaletteExp(CFX_GifImage* gif_image) const;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_GIF_CFX_GIFCONTEXT_H_
