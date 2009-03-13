/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardHistory.cxx

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

/// \file vtkQtChartKeyboardHistory.cxx
/// \date February 23, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartKeyboardHistory.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"


//-----------------------------------------------------------------------------
vtkQtChartKeyboardHistory::vtkQtChartKeyboardHistory(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
}

void vtkQtChartKeyboardHistory::activate()
{
  if(this->Chart)
    {
    vtkQtChartContentsSpace *space = this->Chart->getContentsSpace();
    space->historyPrevious();
    }
}


//-----------------------------------------------------------------------------
vtkQtChartKeyboardHistoryNext::vtkQtChartKeyboardHistoryNext(
    QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
}

void vtkQtChartKeyboardHistoryNext::activate()
{
  if(this->Chart)
    {
    vtkQtChartContentsSpace *space = this->Chart->getContentsSpace();
    space->historyNext();
    }
}


