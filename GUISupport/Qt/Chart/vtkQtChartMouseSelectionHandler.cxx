/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseSelectionHandler.cxx

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

/// \file vtkQtChartMouseSelectionHandler.cxx
/// \date March 19, 2008

#include "vtkQtChartMouseSelectionHandler.h"

#include "vtkQtChartMouseBox.h"


vtkQtChartMouseSelectionHandler::vtkQtChartMouseSelectionHandler(
    QObject *parentObject)
  : QObject(parentObject)
{
  this->MouseBox = 0;
}


