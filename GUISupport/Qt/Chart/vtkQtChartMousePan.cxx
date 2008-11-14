/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMousePan.cxx

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

/// \file vtkQtChartMousePan.cxx
/// \date March 11, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartMousePan.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"
#include <QCursor>
#include <QMouseEvent>
#include <QPointF>


class vtkQtChartMousePanInternal
{
public:
  vtkQtChartMousePanInternal();
  ~vtkQtChartMousePanInternal() {}

  QPoint Last;
  bool LastSet;
};


//----------------------------------------------------------------------------
vtkQtChartMousePanInternal::vtkQtChartMousePanInternal()
  : Last()
{
  this->LastSet = false;
}


//----------------------------------------------------------------------------
vtkQtChartMousePan::vtkQtChartMousePan(QObject *parentObject)
  : vtkQtChartMouseFunction(parentObject)
{
  this->Internal = new vtkQtChartMousePanInternal();
}

vtkQtChartMousePan::~vtkQtChartMousePan()
{
  delete this->Internal;
}

void vtkQtChartMousePan::setMouseOwner(bool owns)
{
  vtkQtChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(QCursor(Qt::ClosedHandCursor));
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool vtkQtChartMousePan::mousePressEvent(QMouseEvent *e, vtkQtChartArea *)
{
  this->Internal->Last = e->globalPos();
  this->Internal->LastSet = true;
  return false;
}

bool vtkQtChartMousePan::mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart)
{
  vtkQtChartContentsSpace *contents = chart->getContentsSpace();
  if(!this->isMouseOwner())
    {
    emit this->interactionStarted(this);
    }

  if(this->isMouseOwner())
    {
    if(this->Internal->LastSet)
      {
      if(!contents->isInInteraction())
        {
        contents->startInteraction();
        }

      QPoint pos = e->globalPos();
      float xDelta = this->Internal->Last.x() - pos.x();
      float yDelta = this->Internal->Last.y() - pos.y();
      contents->setXOffset(xDelta + contents->getXOffset());
      contents->setYOffset(yDelta + contents->getYOffset());
      this->Internal->Last = pos;
      }
    else
      {
      this->Internal->Last = e->globalPos();
      this->Internal->LastSet = true;
      }
    }

  return true;
}

bool vtkQtChartMousePan::mouseReleaseEvent(QMouseEvent *,
    vtkQtChartArea *chart)
{
  vtkQtChartContentsSpace *contents = chart->getContentsSpace();
  if(this->isMouseOwner())
    {
    contents->finishInteraction();
    emit this->interactionFinished(this);
    }

  this->Internal->LastSet = false;
  return true;
}

bool vtkQtChartMousePan::mouseDoubleClickEvent(QMouseEvent *,
    vtkQtChartArea *)
{
  return false;
}


