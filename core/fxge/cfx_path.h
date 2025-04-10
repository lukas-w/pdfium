// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_PATH_H_
#define CORE_FXGE_CFX_PATH_H_

#include <stdint.h>

#include <optional>
#include <vector>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"

class CFX_Path {
 public:
  class Point {
   public:
    enum class Type : uint8_t { kLine, kBezier, kMove };

    Point();
    Point(const CFX_PointF& point, Type type, bool close);
    Point(const Point& other);
    ~Point();

    bool IsTypeAndOpen(Type type) const {
      return type_ == type && !close_figure_;
    }

    CFX_PointF point_;
    Type type_ = Type::kLine;
    bool close_figure_ = false;
  };

  CFX_Path();
  CFX_Path(const CFX_Path& src);
  CFX_Path(CFX_Path&& src) noexcept;
  ~CFX_Path();

  void Clear();

  Point::Type GetType(size_t index) const { return points_[index].type_; }
  bool IsClosingFigure(size_t index) const {
    return points_[index].close_figure_;
  }
  CFX_PointF GetPoint(size_t index) const { return points_[index].point_; }
  const std::vector<Point>& GetPoints() const { return points_; }
  std::vector<Point>& GetPoints() { return points_; }

  CFX_FloatRect GetBoundingBox() const;
  CFX_FloatRect GetBoundingBoxForStrokePath(float line_width,
                                            float miter_limit) const;

  void Transform(const CFX_Matrix& matrix);
  bool IsRect() const;
  std::optional<CFX_FloatRect> GetRect(const CFX_Matrix* matrix) const;

  void Append(const CFX_Path& src, const CFX_Matrix* matrix);
  void AppendFloatRect(const CFX_FloatRect& rect);
  void AppendRect(float left, float bottom, float right, float top);
  void AppendLine(const CFX_PointF& pt1, const CFX_PointF& pt2);
  void AppendPoint(const CFX_PointF& point, Point::Type type);
  void AppendPointAndClose(const CFX_PointF& point, Point::Type type);
  void ClosePath();

 private:
  std::vector<Point> points_;
};

class CFX_RetainablePath final : public Retainable, public CFX_Path {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

  RetainPtr<CFX_RetainablePath> Clone() const;

 private:
  CFX_RetainablePath();
  CFX_RetainablePath(const CFX_RetainablePath& src);
  ~CFX_RetainablePath() override;
};

#endif  // CORE_FXGE_CFX_PATH_H_
