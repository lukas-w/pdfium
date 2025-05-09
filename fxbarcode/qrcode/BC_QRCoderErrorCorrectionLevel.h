// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_QRCODE_BC_QRCODERERRORCORRECTIONLEVEL_H_
#define FXBARCODE_QRCODE_BC_QRCODERERRORCORRECTIONLEVEL_H_

#include <stdint.h>

class CBC_QRCoderErrorCorrectionLevel {
 public:
  static CBC_QRCoderErrorCorrectionLevel* L;
  static CBC_QRCoderErrorCorrectionLevel* M;
  static CBC_QRCoderErrorCorrectionLevel* Q;
  static CBC_QRCoderErrorCorrectionLevel* H;

  static void Initialize();
  static void Finalize();

  ~CBC_QRCoderErrorCorrectionLevel();

  int32_t Ordinal() const { return ordinal_; }
  int32_t GetBits() const { return bits_; }

 private:
  CBC_QRCoderErrorCorrectionLevel(int32_t ordinal, int32_t bits);

  int32_t ordinal_;
  int32_t bits_;
};

#endif  // FXBARCODE_QRCODE_BC_QRCODERERRORCORRECTIONLEVEL_H_
