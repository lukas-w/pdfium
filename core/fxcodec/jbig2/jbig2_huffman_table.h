// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_JBIG2_JBIG2_HUFFMAN_TABLE_H_
#define CORE_FXCODEC_JBIG2_JBIG2_HUFFMAN_TABLE_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "core/fxcodec/jbig2/jbig2_define.h"
#include "core/fxcrt/raw_span.h"

class CJBig2_BitStream;

struct JBig2TableLine {
  uint8_t PREFLEN;
  uint8_t RANGELEN;
  int32_t RANGELOW;
};

struct HuffmanTable {
  bool HTOOB;

  // If HTOOB is set: The last line is for the OOB symbol.
  // Its RANGELEN and RANGELOW are ignored.
  // The last two lines (before the OOB symbol line, if present)
  // are for the -infinity...value range and the value...infinity range.
  // If this table has no -infinity...value range, set its PREFLEN to 0.
  pdfium::raw_span<const JBig2TableLine> lines;
};

class CJBig2_HuffmanTable {
 public:
  explicit CJBig2_HuffmanTable(size_t idx);
  explicit CJBig2_HuffmanTable(CJBig2_BitStream* pStream);

  // Creates a huffman table that maps a canonical code with length
  // prefix_lengths[i] to i.
  explicit CJBig2_HuffmanTable(pdfium::span<uint8_t> prefix_lengths);
  ~CJBig2_HuffmanTable();

  bool IsHTOOB() const { return HTOOB; }
  uint32_t Size() const { return NTEMP; }
  const std::vector<JBig2HuffmanCode>& GetCODES() const { return CODES; }
  const std::vector<int>& GetRANGELEN() const { return RANGELEN; }
  const std::vector<int>& GetRANGELOW() const { return RANGELOW; }
  bool IsOK() const { return ok_; }

  static constexpr size_t kNumHuffmanTables = 16;

 private:
  bool ParseFromTable(const HuffmanTable& table);
  bool ParseFromCodedBuffer(CJBig2_BitStream* pStream);
  void ExtendBuffers(bool increment);

  bool ok_;
  bool HTOOB;
  uint32_t NTEMP;
  std::vector<JBig2HuffmanCode> CODES;
  std::vector<int> RANGELEN;
  std::vector<int> RANGELOW;
};

#endif  // CORE_FXCODEC_JBIG2_JBIG2_HUFFMAN_TABLE_H_
