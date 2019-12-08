/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#ifndef QVTK_PAINT_ENGINE_HPP
#define QVTK_PAINT_ENGINE_HPP

#include "QVTKWin32Header.h"
#include <QPaintEngine>
#include <vtkSetGet.h>
class QVTKWidget;
class QVTKPaintEngineInternal;

/**
 * @class QVTKPaintEngine
 * @brief directs QPainter calls to a VTK window
 *
 * A paint engine class to direct QPainter calls into a VTK window.
 * @deprecated Only used in conjunction with QVTKWidget which has also been
 * deprecated.
 */
class QVTKPaintEngine : public QPaintEngine
{
public:
  VTK_LEGACY(QVTKPaintEngine());
  ~QVTKPaintEngine() override;

  // Description:
  // begin painting on device (QVTKWidget)
  bool begin(QPaintDevice* dev) override;

  // Description:
  // end painting on device
  bool end() override;

  // Description:
  // returns type User
  QPaintEngine::Type type() const override;

  // Description:
  // updateState
  void updateState(const QPaintEngineState&) override;

  // Description:
  // draw a pixmap
  void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) override;

  // Description:
  // draw a path
  void drawPath(const QPainterPath& path) override;

  // Description:
  // draw a polygon
  void drawPolygon(const QPointF* points, int pointCount, PolygonDrawMode mode) override;
  void drawPolygon(const QPoint* points, int pointCount, PolygonDrawMode mode) override;

protected:
  QVTKWidget* Widget;
  QVTKPaintEngineInternal* Internal;
};

#endif
