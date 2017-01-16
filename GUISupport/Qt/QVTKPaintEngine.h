/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

// .NAME QVTKPaintEngine - directs QPainter calls to a VTK window

#ifndef QVTK_PAINT_ENGINE_HPP
#define QVTK_PAINT_ENGINE_HPP

#include "QVTKWin32Header.h"
#include <QPaintEngine>
class QVTKWidget;
class QVTKPaintEngineInternal;

//!  A paint engine class to direct QPainter calls into a VTK window
class QVTKPaintEngine : public QPaintEngine
{
public:
  QVTKPaintEngine();
  ~QVTKPaintEngine() VTK_OVERRIDE;

  // Description:
  // begin painting on device (QVTKWidget)
  bool begin(QPaintDevice* dev) VTK_OVERRIDE;

  // Description:
  // end painting on device
  bool end() VTK_OVERRIDE;

  // Description:
  // returns type User
  QPaintEngine::Type type() const VTK_OVERRIDE;

  // Description:
  // updateState
  void updateState(const QPaintEngineState&) VTK_OVERRIDE;

  // Description:
  // draw a pixmap
  void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) VTK_OVERRIDE;

  // Description:
  // draw a path
  void drawPath(const QPainterPath& path) VTK_OVERRIDE;

  // Description:
  // draw a polygon
  void drawPolygon(const QPointF* points, int pointCount, PolygonDrawMode mode) VTK_OVERRIDE;
  void drawPolygon(const QPoint* points, int pointCount, PolygonDrawMode mode) VTK_OVERRIDE;

protected:

  QVTKWidget* Widget;
  QVTKPaintEngineInternal* Internal;
};

#endif

