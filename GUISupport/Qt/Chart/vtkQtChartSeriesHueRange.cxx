/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesHueRange.cxx

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

/// \file vtkQtChartSeriesHueRange.cxx
/// \date February 26, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesHueRange.h"

#include "vtkQtChartColors.h"
#include <QBrush>
#include <QColor>


vtkQtChartSeriesHueRange::vtkQtChartSeriesHueRange(QObject *parentObject)
  : vtkQtChartSeriesColors(parentObject)
{
  this->First = new QColor(Qt::red);
  this->Second = new QColor(Qt::blue);
}

vtkQtChartSeriesHueRange::~vtkQtChartSeriesHueRange()
{
  delete this->First;
  delete this->Second;
}

void vtkQtChartSeriesHueRange::getBrush(int index, int total,
    QBrush &brush) const
{
  // Interpolate the HSV values.
  float fraction = (float)index / (float)total;
  brush.setColor(vtkQtChartColors::interpolateHsv(*this->First, *this->Second,
      fraction));
}

void vtkQtChartSeriesHueRange::setRange(const QColor &color1,
    const QColor &color2)
{
  *this->First = color1;
  *this->Second = color2;
}


