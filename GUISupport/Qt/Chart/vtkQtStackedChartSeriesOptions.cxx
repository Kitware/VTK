/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChartSeriesOptions.cxx

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

/// \file vtkQtStackedChartSeriesOptions.cxx
/// \date February 27, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtStackedChartSeriesOptions.h"

#include "vtkQtChartStyleGenerator.h"
#include <QBrush>
#include <QColor>
#include <QPen>


vtkQtStackedChartSeriesOptions::vtkQtStackedChartSeriesOptions(
    QObject *parentObject)
  : vtkQtChartSeriesOptions(parentObject)
{
  this->setBrush(Qt::red);
}

void vtkQtStackedChartSeriesOptions::setStyle(int style,
                                              vtkQtChartStyleGenerator *generator)
{
  vtkQtChartSeriesOptions::setStyle(style, generator);
  if(generator)
    {
    QBrush brush = generator->getSeriesBrush(style);
    this->setBrush(brush);
    this->setPen(QPen(brush.color().dark()));
    }
}


