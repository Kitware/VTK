/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardZoom.cxx

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

/// \file vtkQtChartKeyboardZoom.cxx
/// \date February 23, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartKeyboardZoom.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoom::vtkQtChartKeyboardZoom(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
  this->Flags = vtkQtChartKeyboardZoom::ZoomBoth;
  this->Method = vtkQtChartKeyboardZoom::ZoomIn;
}

void vtkQtChartKeyboardZoom::activate()
{
  if(this->Chart)
    {
    bool changeInX = true;
    bool changeInY = true;
    if(this->Flags == vtkQtChartKeyboardZoom::ZoomXOnly)
      {
      changeInY = false;
      }
    else if(this->Flags == vtkQtChartKeyboardZoom::ZoomYOnly)
      {
      changeInX = false;
      }

    vtkQtChartContentsSpace *space = this->Chart->getContentsSpace();
    float step = space->getZoomFactorStep();
    float zx = space->getXZoomFactor();
    float zy = space->getYZoomFactor();
    if(changeInX)
      {
      if(this->Method == vtkQtChartKeyboardZoom::ZoomIn)
        {
        zx += step;
        }
      else
        {
        zx -= step;
        }
      }

    if(changeInY)
      {
      if(this->Method == vtkQtChartKeyboardZoom::ZoomIn)
        {
        zy += step;
        }
      else
        {
        zy -= step;
        }
      }

    space->zoomToFactor(zx, zy);
    }
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoomX::vtkQtChartKeyboardZoomX(QObject *parentObject)
  : vtkQtChartKeyboardZoom(parentObject)
{
  this->setZoomFlags(vtkQtChartKeyboardZoom::ZoomXOnly);
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoomY::vtkQtChartKeyboardZoomY(QObject *parentObject)
  : vtkQtChartKeyboardZoom(parentObject)
{
  this->setZoomFlags(vtkQtChartKeyboardZoom::ZoomYOnly);
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoomOut::vtkQtChartKeyboardZoomOut(QObject *parentObject)
  : vtkQtChartKeyboardZoom(parentObject)
{
  this->setZoomMethod(vtkQtChartKeyboardZoom::ZoomOut);
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoomOutX::vtkQtChartKeyboardZoomOutX(QObject *parentObject)
  : vtkQtChartKeyboardZoomOut(parentObject)
{
  this->setZoomFlags(vtkQtChartKeyboardZoom::ZoomXOnly);
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardZoomOutY::vtkQtChartKeyboardZoomOutY(QObject *parentObject)
  : vtkQtChartKeyboardZoomOut(parentObject)
{
  this->setZoomFlags(vtkQtChartKeyboardZoom::ZoomYOnly);
}


