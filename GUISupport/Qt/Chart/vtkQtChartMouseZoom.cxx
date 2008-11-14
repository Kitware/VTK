/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseZoom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartMouseZoom.cxx
/// \date March 11, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartMouseZoom.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartMouseBox.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPixmap>
#include <QPointF>
#include <QRectF>

// TODO: Use Qt resource instead of including directly.
// Zoom cursor xpm.
#include "zoom.xpm"


class vtkQtChartMouseZoomInternal
{
public:
  vtkQtChartMouseZoomInternal();
  ~vtkQtChartMouseZoomInternal() {}

  QCursor ZoomCursor;
  QPoint Last;
  bool LastSet;
};


//----------------------------------------------------------------------------
vtkQtChartMouseZoomInternal::vtkQtChartMouseZoomInternal()
  : ZoomCursor(QPixmap(zoom_xpm), 11, 11), Last()
{
  this->LastSet = false;
}


//----------------------------------------------------------------------------
vtkQtChartMouseZoom::vtkQtChartMouseZoom(QObject *parentObject)
  : vtkQtChartMouseFunction(parentObject)
{
  this->Internal = new vtkQtChartMouseZoomInternal();
  this->Flags = vtkQtChartMouseZoom::ZoomBoth;
}

vtkQtChartMouseZoom::~vtkQtChartMouseZoom()
{
  delete this->Internal;
}

void vtkQtChartMouseZoom::setMouseOwner(bool owns)
{
  vtkQtChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(this->Internal->ZoomCursor);
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool vtkQtChartMouseZoom::mousePressEvent(QMouseEvent *e, vtkQtChartArea *)
{
  this->Internal->Last = e->globalPos();
  this->Internal->LastSet = true;
  return false;
}

bool vtkQtChartMouseZoom::mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart)
{
  if(!this->isMouseOwner())
    {
    emit this->interactionStarted(this);
    }

  if(this->isMouseOwner())
    {
    if(this->Internal->LastSet)
      {
      vtkQtChartContentsSpace *contents = chart->getContentsSpace();
      if(!contents->isInInteraction())
        {
        contents->startInteraction();
        chart->startInteractiveResize();
        }

      // Zoom in or out based on the mouse movement up or down.
      QPoint pos = e->globalPos();
      int delta = (this->Internal->Last.y() - pos.y())/4;
      if(delta != 0)
        {
        float x = contents->getXZoomFactor();
        float y = contents->getYZoomFactor();
        if(this->Flags != vtkQtChartMouseZoom::ZoomYOnly)
          {
          x += (float)delta / 100.0;
          }

        if(this->Flags != vtkQtChartMouseZoom::ZoomXOnly)
          {
          y += (float)delta / 100.0;
          }

        this->Internal->Last = pos;
        contents->zoomToFactor(x, y);
        }
      }
    else
      {
      this->Internal->Last = e->globalPos();
      this->Internal->LastSet = true;
      }
    }

  return true;
}

bool vtkQtChartMouseZoom::mouseReleaseEvent(QMouseEvent *,
    vtkQtChartArea *chart)
{
  if(this->isMouseOwner())
    {
    chart->getContentsSpace()->finishInteraction();
    chart->finishInteractiveResize();
    emit this->interactionFinished(this);
    }

  return true;
}

bool vtkQtChartMouseZoom::mouseDoubleClickEvent(QMouseEvent *,
    vtkQtChartArea *chart)
{
  chart->getContentsSpace()->resetZoom();
  return true;
}

bool vtkQtChartMouseZoom::wheelEvent(QWheelEvent *e,
    vtkQtChartArea *chart)
{
  // If the wheel event delta is positive, zoom in. Otherwise, zoom
  // out.
  vtkQtChartContentsSpace *contents = chart->getContentsSpace();
  float factorChange = vtkQtChartContentsSpace::getZoomFactorStep();
  if(e->delta() < 0)
    {
    factorChange *= -1;
    }

  QPoint point = e->pos();
  float x = (float)point.x() + contents->getXOffset();
  float y = (float)point.y() + contents->getYOffset();
  float oldXZoom = contents->getXZoomFactor();
  float oldYZoom = contents->getYZoomFactor();
  float newXZoom = oldXZoom;
  float newYZoom = oldYZoom;
  if(this->Flags != vtkQtChartMouseZoom::ZoomYOnly)
    {
    newXZoom += factorChange;
    }

  if(this->Flags != vtkQtChartMouseZoom::ZoomXOnly)
    {
    newYZoom += factorChange;
    }

  bool interact = contents->isInInteraction();
  if(!interact)
    {
    contents->startInteraction();
    }

  // Set the new zoom factor(s).
  contents->zoomToFactor(newXZoom, newYZoom);
  newXZoom = contents->getXZoomFactor();
  newYZoom = contents->getYZoomFactor();

  // Keep the same position under the point if possible.
  if(this->Flags != vtkQtChartMouseZoom::ZoomYOnly && newXZoom != oldXZoom)
    {
    x = (newXZoom * x)/(oldXZoom);
    }

  x -= point.x();
  contents->setXOffset(x);

  if(this->Flags != vtkQtChartMouseZoom::ZoomXOnly && newYZoom != oldYZoom)
    {
    y = (newYZoom * y)/(oldYZoom);
    }

  y -= point.y();
  contents->setYOffset(y);

  if(!interact)
    {
    contents->finishInteraction();
    }

  return true;
}


//----------------------------------------------------------------------------
vtkQtChartMouseZoomX::vtkQtChartMouseZoomX(QObject *parentObject)
  : vtkQtChartMouseZoom(parentObject)
{
  this->setFlags(vtkQtChartMouseZoom::ZoomXOnly);
}


//----------------------------------------------------------------------------
vtkQtChartMouseZoomY::vtkQtChartMouseZoomY(QObject *parentObject)
  : vtkQtChartMouseZoom(parentObject)
{
  this->setFlags(vtkQtChartMouseZoom::ZoomYOnly);
}


//----------------------------------------------------------------------------
vtkQtChartMouseZoomBox::vtkQtChartMouseZoomBox(QObject *parentObject)
  : vtkQtChartMouseFunction(parentObject)
{
  this->ZoomCursor = new QCursor(QPixmap(zoom_xpm), 11, 11);
}

vtkQtChartMouseZoomBox::~vtkQtChartMouseZoomBox()
{
  delete this->ZoomCursor;
}

void vtkQtChartMouseZoomBox::setMouseOwner(bool owns)
{
  vtkQtChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(*this->ZoomCursor);
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool vtkQtChartMouseZoomBox::mousePressEvent(QMouseEvent *, vtkQtChartArea *)
{
  return false;
}

bool vtkQtChartMouseZoomBox::mouseMoveEvent(QMouseEvent *e,
    vtkQtChartArea *chart)
{
  vtkQtChartMouseBox *mouseBox = chart->getMouseBox();
  if(!this->isMouseOwner() && mouseBox)
    {
    emit this->interactionStarted(this);
    mouseBox->setVisible(true);
    }

  if(this->isMouseOwner())
    {
    mouseBox->adjustRectangle(e->pos());
    }

  return true;
}

bool vtkQtChartMouseZoomBox::mouseReleaseEvent(QMouseEvent *e,
    vtkQtChartArea *chart)
{
  if(this->isMouseOwner())
    {
    // Adjust the mouse box before using it.
    vtkQtChartMouseBox *mouseBox = chart->getMouseBox();
    mouseBox->adjustRectangle(e->pos());
    mouseBox->setVisible(false);

    // Get the mouse box rectangle in scene coordinates.
    QRectF area = mouseBox->getRectangle();

    // Make sure the area and contents are valid.
    QRectF bounds;
    vtkQtChartContentsSpace *contents = chart->getContentsSpace();
    contents->getChartLayerBounds(bounds);
    float width = contents->getChartWidth();
    float height = contents->getChartHeight();
    if(area.isValid() && area.x() >= 0.0 && area.y() >= 0.0 &&
        width >= 0.0 && height >= 0.0 && bounds.isValid())
      {
      // Adjust the top-left corner coordinates for the chart layer
      // viewport and the current offset.
      float x = area.x() - bounds.x() + contents->getXOffset();
      float y = area.y() - bounds.y() + contents->getYOffset();

      // Find the new zoom factors using the zoom factors for the chart
      // layer viewport.
      float xZoom1 = width * (contents->getXZoomFactor() - 1);
      xZoom1 = (xZoom1 / bounds.width()) + 1;
      float xZoom2 = (xZoom1 * bounds.width()) / area.width();
      float xFactor = bounds.width() * (xZoom2 - 1);
      xFactor = (xFactor / width) + 1;

      float yZoom1 = height * (contents->getYZoomFactor() - 1);
      yZoom1 = (yZoom1 / bounds.height()) + 1;
      float yZoom2 = (yZoom1 * bounds.height()) / area.height();
      float yFactor = bounds.height() * (yZoom2 - 1);
      yFactor = (yFactor / height) + 1;

      // Set the new zoom factors.
      contents->startInteraction();
      contents->zoomToFactor(xFactor, yFactor);

      // Re-calculate the second zoom factors.
      xZoom2 = width * (contents->getXZoomFactor() - 1);
      xZoom2 = (xZoom2 / bounds.width()) + 1;
      yZoom2 = height * (contents->getYZoomFactor() - 1);
      yZoom2 = (yZoom2 / bounds.height()) + 1;

      // Set the offset to match the original zoom area.
      contents->setXOffset((xZoom2 * x) / xZoom1);
      contents->setYOffset((yZoom2 * y) / yZoom1);
      contents->finishInteraction();
      }

    // Notify the interactor that the interaction state is finished.
    emit this->interactionFinished(this);
    }

  return true;
}

bool vtkQtChartMouseZoomBox::mouseDoubleClickEvent(QMouseEvent *,
    vtkQtChartArea *)
{
  return false;
}


