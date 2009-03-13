/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartContentsSpace.cxx

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

/// \file vtkQtChartContentsSpace.cxx
/// \date 2/7/2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartContentsSpace.h"

#include "vtkQtChartZoomHistory.h"
#include <QPointF>
#include <QRectF>


// Set a maximum zoom factor to prevent overflow problems while zooming.
#define MAX_ZOOM 16


class vtkQtChartContentsSpaceInternal
{
public:
  vtkQtChartContentsSpaceInternal();
  ~vtkQtChartContentsSpaceInternal() {}

  QRectF Layer;                  ///< Stores the chart layer viewport.
  vtkQtChartZoomHistory History; ///< Stores the viewport zoom history.
  bool InHistory;                ///< Used for zoom history processing.
  bool InInteraction;            ///< Used for interactive zoom.
};


//----------------------------------------------------------------------------
vtkQtChartContentsSpaceInternal::vtkQtChartContentsSpaceInternal()
  : Layer(), History()
{
  this->InHistory = false;
  this->InInteraction = false;
}


//----------------------------------------------------------------------------
float vtkQtChartContentsSpace::ZoomFactorStep = (float)0.1;
float vtkQtChartContentsSpace::PanStep = (float)15.0;


//----------------------------------------------------------------------------
vtkQtChartContentsSpace::vtkQtChartContentsSpace(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartContentsSpaceInternal();
  this->OffsetX = 0;
  this->OffsetY = 0;
  this->MaximumX = 0;
  this->MaximumY = 0;
  this->Width = 0;
  this->Height = 0;
  this->ZoomFactorX = 1;
  this->ZoomFactorY = 1;

  // Set up the original zoom position in the history.
  this->Internal->History.addHistory(0, 0, 1, 1);
}

vtkQtChartContentsSpace::~vtkQtChartContentsSpace()
{
  delete this->Internal;
}

float vtkQtChartContentsSpace::getContentsWidth() const
{
  return this->Internal->Layer.width() + this->MaximumX;
}

float vtkQtChartContentsSpace::getContentsHeight() const
{
  return this->Internal->Layer.height() + this->MaximumY;
}

void vtkQtChartContentsSpace::translateToLayerContents(QPointF &point) const
{
  point.setX(point.x() + this->OffsetX - this->Internal->Layer.left());
  point.setY(point.y() + this->OffsetY - this->Internal->Layer.top());
}

void vtkQtChartContentsSpace::translateToLayerContents(QRectF &area) const
{
  area.translate(this->OffsetX - this->Internal->Layer.left(),
      this->OffsetY - this->Internal->Layer.top());
}

void vtkQtChartContentsSpace::setChartSize(float width, float height)
{
  if(this->Width != width || this->Height != height)
    {
    // Scale the offsets for the new size.
    bool changeXOffset = this->Width != 0 && this->OffsetX != 0;
    if(changeXOffset)
      {
      this->OffsetX = (this->OffsetX * width) / this->Width;
      }

    bool changeYOffset = this->Height != 0 && this->OffsetY != 0;
    if(changeYOffset)
      {
      this->OffsetY = (this->OffsetY * height) / this->Height;
      }

    // Use the xoom factors to determine the new maximum offsets.
    bool xShrinking = width < this->Width;
    this->Width = width;
    if(this->ZoomFactorX > 1)
      {
      this->MaximumX = (this->Width * this->ZoomFactorX) - this->Width;
      }

    bool yShrinking = height < this->Height;
    this->Height = height;
    if(this->ZoomFactorY > 1)
      {
      this->MaximumY = (this->Height * this->ZoomFactorY) - this->Height;
      }

    if(xShrinking && changeXOffset)
      {
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(yShrinking && changeYOffset)
      {
      emit this->yOffsetChanged(this->OffsetY);
      }

    if(this->ZoomFactorX > 1 || this->ZoomFactorY > 1)
      {
      emit this->maximumChanged(this->MaximumX, this->MaximumY);
      }

    if(!xShrinking && changeXOffset)
      {
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(!yShrinking && changeYOffset)
      {
      emit this->yOffsetChanged(this->OffsetY);
      }
    }
}

void vtkQtChartContentsSpace::getChartLayerBounds(QRectF &bounds) const
{
  bounds = this->Internal->Layer;
}

void vtkQtChartContentsSpace::setChartLayerBounds(const QRectF &bounds)
{
  this->Internal->Layer = bounds;
}

void vtkQtChartContentsSpace::zoomToFactor(float factor)
{
  this->zoomToFactor(factor, factor);
}

void vtkQtChartContentsSpace::zoomToFactor(float xFactor, float yFactor)
{
  if(xFactor < 1)
    {
    xFactor = 1;
    }
  else if(xFactor > MAX_ZOOM)
    {
    xFactor = MAX_ZOOM;
    }

  if(yFactor < 1)
    {
    yFactor = 1;
    }
  else if(yFactor > MAX_ZOOM)
    {
    yFactor = MAX_ZOOM;
    }

  if(this->ZoomFactorX != xFactor || this->ZoomFactorY != yFactor)
    {
    this->ZoomFactorX = xFactor;
    this->ZoomFactorY = yFactor;
    if(this->Width != 0 || this->Height != 0)
      {
      this->MaximumX = (this->Width * this->ZoomFactorX) - this->Width;
      this->MaximumY = (this->Height * this->ZoomFactorY) - this->Height;

      // Make sure the offsets fit in the new space.
      bool interact = this->Internal->InInteraction;
      this->Internal->InInteraction = true;
      this->setXOffset(this->OffsetX);
      this->setYOffset(this->OffsetY);
      this->Internal->InInteraction = interact;

      if(!this->Internal->InHistory && !this->Internal->InInteraction)
        {
        this->addHistory();
        }

      emit this->maximumChanged(this->MaximumX, this->MaximumY);
      }
    }
}

void vtkQtChartContentsSpace::zoomToFactorX(float factor)
{
  this->zoomToFactor(factor, this->ZoomFactorY);
}

void vtkQtChartContentsSpace::zoomToFactorY(float factor)
{
  this->zoomToFactor(this->ZoomFactorX, factor);
}

void vtkQtChartContentsSpace::startInteraction()
{
  this->Internal->InInteraction = true;
}

bool vtkQtChartContentsSpace::isInInteraction() const
{
  return this->Internal->InInteraction;
}

void vtkQtChartContentsSpace::finishInteraction()
{
  if(this->Internal->InInteraction)
    {
    this->Internal->InInteraction = false;

    // If the zoom factors have changed, update the history.
    const vtkQtChartZoomViewport *current =
        this->Internal->History.getCurrent();
    if(!current || (current->getXZoom() != this->ZoomFactorX ||
        current->getYZoom() != this->ZoomFactorY ||
        current->getXPosition() != this->OffsetX ||
        current->getYPosition() != this->OffsetY))
      {
      this->addHistory();
      }
    }
}

bool vtkQtChartContentsSpace::isHistoryPreviousAvailable() const
{
  return this->Internal->History.isPreviousAvailable();
}

bool vtkQtChartContentsSpace::isHistoryNextAvailable() const
{
  return this->Internal->History.isNextAvailable();
}

void vtkQtChartContentsSpace::setXOffset(float offset)
{
  if(offset < 0)
    {
    offset = 0;
    }
  else if(offset > this->MaximumX)
    {
    offset = this->MaximumX;
    }

  if(this->OffsetX != offset)
    {
    this->OffsetX = offset;
    if(!this->Internal->InHistory && !this->Internal->InInteraction)
      {
      this->addHistory();
      }
      
    emit this->xOffsetChanged(this->OffsetX);
    }
}

void vtkQtChartContentsSpace::setYOffset(float offset)
{
  if(offset < 0)
    {
    offset = 0;
    }
  else if(offset > this->MaximumY)
    {
    offset = this->MaximumY;
    }

  if(this->OffsetY != offset)
    {
    this->OffsetY = offset;
    if(!this->Internal->InHistory && !this->Internal->InInteraction)
      {
      this->addHistory();
      }
      
    emit this->yOffsetChanged(this->OffsetY);
    }
}

void vtkQtChartContentsSpace::setMaximumXOffset(float maximum)
{
  if(this->MaximumX != maximum && maximum >= 0)
    {
    this->MaximumX = maximum;
    if(this->OffsetX > this->MaximumX)
      {
      this->OffsetX = this->MaximumX;
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(this->Width != 0)
      {
      this->ZoomFactorX = (this->Width + this->MaximumX) / this->Width;
      }

    emit this->maximumChanged(this->MaximumX, this->MaximumY);
    }
}

void vtkQtChartContentsSpace::setMaximumYOffset(float maximum)
{
  if(this->MaximumY != maximum && maximum >= 0)
    {
    this->MaximumY = maximum;
    if(this->OffsetY > this->MaximumY)
      {
      this->OffsetY = this->MaximumY;
      emit this->yOffsetChanged(this->OffsetY);
      }

    if(this->Height != 0)
      {
      this->ZoomFactorY = (this->Height + this->MaximumY) / this->Height;
      }

    emit this->maximumChanged(this->MaximumX, this->MaximumY);
    }
}

void vtkQtChartContentsSpace::panUp()
{
  this->setYOffset(this->OffsetY - vtkQtChartContentsSpace::PanStep);
}

void vtkQtChartContentsSpace::panDown()
{
  this->setYOffset(this->OffsetY + vtkQtChartContentsSpace::PanStep);
}

void vtkQtChartContentsSpace::panLeft()
{
  this->setXOffset(this->OffsetX - vtkQtChartContentsSpace::PanStep);
}

void vtkQtChartContentsSpace::panRight()
{
  this->setXOffset(this->OffsetX + vtkQtChartContentsSpace::PanStep);
}

void vtkQtChartContentsSpace::resetZoom()
{
  this->zoomToFactor(1, 1);
}

void vtkQtChartContentsSpace::historyNext()
{
  const vtkQtChartZoomViewport *zoom = this->Internal->History.getNext();
  if(zoom)
    {
    this->Internal->InHistory = true;
    this->zoomToFactor(zoom->getXZoom(), zoom->getYZoom());
    this->setXOffset(zoom->getXPosition());
    this->setYOffset(zoom->getYPosition());
    this->Internal->InHistory = false;

    emit this->historyPreviousAvailabilityChanged(
        this->Internal->History.isPreviousAvailable());
    emit this->historyNextAvailabilityChanged(
        this->Internal->History.isNextAvailable());
    }
}

void vtkQtChartContentsSpace::historyPrevious()
{
  const vtkQtChartZoomViewport *zoom = this->Internal->History.getPrevious();
  if(zoom)
    {
    this->Internal->InHistory = true;
    this->zoomToFactor(zoom->getXZoom(), zoom->getYZoom());
    this->setXOffset(zoom->getXPosition());
    this->setYOffset(zoom->getYPosition());
    this->Internal->InHistory = false;

    emit this->historyPreviousAvailabilityChanged(
        this->Internal->History.isPreviousAvailable());
    emit this->historyNextAvailabilityChanged(
        this->Internal->History.isNextAvailable());
    }
}

float vtkQtChartContentsSpace::getZoomFactorStep()
{
  return vtkQtChartContentsSpace::ZoomFactorStep;
}

void vtkQtChartContentsSpace::setZoomFactorStep(float step)
{
  vtkQtChartContentsSpace::ZoomFactorStep = step;
}

float vtkQtChartContentsSpace::getPanStep()
{
  return vtkQtChartContentsSpace::PanStep;
}

void vtkQtChartContentsSpace::setPanStep(float step)
{
  vtkQtChartContentsSpace::PanStep = step;
}

void vtkQtChartContentsSpace::addHistory()
{
  // Add the new zoom location to the zoom history.
  this->Internal->History.addHistory(this->OffsetX, this->OffsetY,
      this->ZoomFactorX, this->ZoomFactorY);
  emit this->historyPreviousAvailabilityChanged(
      this->Internal->History.isPreviousAvailable());
  emit this->historyNextAvailabilityChanged(
      this->Internal->History.isNextAvailable());
}


