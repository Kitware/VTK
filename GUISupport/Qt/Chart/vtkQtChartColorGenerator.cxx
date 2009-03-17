/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColorGenerator.cxx

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

/// \file vtkQtChartColorGenerator.cxx
/// \date March 16, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartColorGenerator.h"

#include "vtkQtChartColors.h"


vtkQtChartColorGenerator::vtkQtChartColorGenerator(QObject *parentObject)
  : vtkQtChartStyleBrush(parentObject)
{
  this->Colors = 0;
}

QBrush vtkQtChartColorGenerator::getStyleBrush(int index) const
{
  if(index >= 0 && this->Colors && this->Colors->getNumberOfColors() > 0)
    {
    index = index % this->Colors->getNumberOfColors();
    return QBrush(this->Colors->getColor(index));
    }

  return QBrush();
}


