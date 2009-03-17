/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartSeriesOptions.cxx

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

/// \file vtkQtBarChartSeriesOptions.cxx
/// \date February 22, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtBarChartSeriesOptions.h"

#include <QBrush>


vtkQtBarChartSeriesOptions::vtkQtBarChartSeriesOptions(QObject *parentObject)
  : vtkQtChartSeriesOptions(parentObject)
{
  this->MultiColored = false;
  this->setBrush(QBrush(Qt::red));
}

void vtkQtBarChartSeriesOptions::setMultiColored(bool multiColored)
{
  if(this->MultiColored != multiColored)
    {
    this->MultiColored = multiColored;
    emit this->multiColoredChanged(multiColored);
    }
}


