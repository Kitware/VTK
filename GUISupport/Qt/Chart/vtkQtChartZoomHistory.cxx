/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartZoomHistory.cxx

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

/// \file vtkQtChartZoomHistory.cxx
/// \date 2/7/2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartZoomHistory.h"

#include <QVector>


class vtkQtChartZoomHistoryInternal : public QVector<vtkQtChartZoomViewport *>
{
};


//-----------------------------------------------------------------------------
vtkQtChartZoomViewport::vtkQtChartZoomViewport()
{
  this->X = 0;
  this->Y = 0;
  this->XFactor = 1;
  this->YFactor = 1;
}

void vtkQtChartZoomViewport::setPosition(float x, float y)
{
  this->X = x;
  this->Y = y;
}

void vtkQtChartZoomViewport::setZoom(float x, float y)
{
  this->XFactor = x;
  this->YFactor = y;
}


//-----------------------------------------------------------------------------
vtkQtChartZoomHistory::vtkQtChartZoomHistory()
{
  this->Internal = new vtkQtChartZoomHistoryInternal();
  this->Current = 0;
  this->Allowed = 10;

  // Reserve space for the history list.
  this->Internal->reserve(this->Allowed);
}

vtkQtChartZoomHistory::~vtkQtChartZoomHistory()
{
  QVector<vtkQtChartZoomViewport *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); iter++)
    {
    delete *iter;
    }

  delete this->Internal;
}

void vtkQtChartZoomHistory::setLimit(int limit)
{
  if(limit > 0)
    {
    this->Allowed = limit;
    this->Internal->reserve(this->Allowed);
    }
}

void vtkQtChartZoomHistory::addHistory(
    float x, float y, float xZoom, float yZoom)
{
  vtkQtChartZoomViewport *zoom = new vtkQtChartZoomViewport();
  zoom->setPosition(x, y);
  zoom->setZoom(xZoom, yZoom);

  // Remove history items after the current one.
  if(this->Internal->size() >= this->Allowed ||
      this->Current < this->Internal->size() - 1)
    {
    int front = this->Internal->size() - this->Allowed + 1;
    if(this->Current < this->Allowed - 1)
      {
      front = 0;
      }

    QVector<vtkQtChartZoomViewport *>::Iterator iter = this->Internal->begin();
    for(int i = 0; iter != this->Internal->end(); ++iter, ++i)
      {
      if(i < front || i > this->Current)
        {
        delete *iter;
        *iter = 0;
        }
      }

    // First, remove the empty items from the end.
    if(this->Current < this->Internal->size() - 1)
      {
      this->Internal->resize(this->Current + 1);
      }

    // Remove any empty items from the front of the list.
    if(front > 0)
      {
      this->Internal->remove(0, front);
      }
    }

  // Add the zoom item to the end of the list and update the current
  // position.
  this->Internal->append(zoom);
  this->Current = this->Internal->size() - 1;
}

void vtkQtChartZoomHistory::updatePosition(float x, float y)
{
  if(this->Current < this->Internal->size())
    {
    vtkQtChartZoomViewport *zoom = (*this->Internal)[this->Current];
    zoom->setPosition(x, y);
    }
}

bool vtkQtChartZoomHistory::isPreviousAvailable() const
{
  return this->Current > 0;
}

bool vtkQtChartZoomHistory::isNextAvailable() const
{
  return this->Current < this->Internal->size() - 1;
}

const vtkQtChartZoomViewport *vtkQtChartZoomHistory::getCurrent() const
{
  if(this->Current < this->Internal->size())
    {
    return (*this->Internal)[this->Current];
    }

  return 0;
}

const vtkQtChartZoomViewport *vtkQtChartZoomHistory::getPrevious()
{
  this->Current--;
  if(this->Current < 0)
    {
    this->Current = 0;
    return 0;
    }
  else
    {
    return this->getCurrent();
    }
}

const vtkQtChartZoomViewport *vtkQtChartZoomHistory::getNext()
{
  this->Current++;
  if(this->Current < this->Internal->size())
    {
    return this->getCurrent();
    }
  else
    {
    if(this->Current > 0)
      {
      this->Current--;
      }

    return 0;
    }
}


