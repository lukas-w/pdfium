// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_PICTUREBOX_H_
#define XFA_FWL_CFWL_PICTUREBOX_H_

#include "xfa/fwl/cfwl_widget.h"

namespace pdfium {

class CFWL_PictureBox final : public CFWL_Widget {
 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CFWL_PictureBox() override;

  // CFWL_Widget
  FWL_Type GetClassID() const override;
  void Update() override;
  void DrawWidget(CFGAS_GEGraphics* pGraphics,
                  const CFX_Matrix& matrix) override;
  void OnDrawWidget(CFGAS_GEGraphics* pGraphics,
                    const CFX_Matrix& matrix) override;

 private:
  explicit CFWL_PictureBox(CFWL_App* pApp);

  CFX_RectF client_rect_;
  CFX_RectF image_rect_;
  CFX_Matrix matrix_;
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_PictureBox;

#endif  // XFA_FWL_CFWL_PICTUREBOX_H_
