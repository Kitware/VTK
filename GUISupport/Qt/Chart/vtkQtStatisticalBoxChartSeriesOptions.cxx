/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartSeriesOptions.cxx

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

/// \file vtkQtStatisticalBoxChartSeriesOptions.cxx
/// \date May 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtStatisticalBoxChartSeriesOptions.h"

#include <QBrush>


vtkQtStatisticalBoxChartSeriesOptions::vtkQtStatisticalBoxChartSeriesOptions(
    QObject *parentObject)
  : vtkQtChartSeriesOptions(parentObject)
{
  this->PointStyle = vtkQtPointMarker::Circle;
  this->PointSize = new QSizeF(5.0, 5.0);

  this->setBrush(Qt::red);
}

vtkQtStatisticalBoxChartSeriesOptions::~vtkQtStatisticalBoxChartSeriesOptions()
{
  delete this->PointSize;
}

vtkQtPointMarker::MarkerStyle
vtkQtStatisticalBoxChartSeriesOptions::getMarkerStyle() const
{
  return this->PointStyle;
}

void vtkQtStatisticalBoxChartSeriesOptions::setMarkerStyle(
    vtkQtPointMarker::MarkerStyle style)
{
  if(style != this->PointStyle)
    {
    this->PointStyle = style;
    emit this->pointMarkerChanged();
    }
}

const QSizeF &vtkQtStatisticalBoxChartSeriesOptions::getMarkerSize() const
{
  return *this->PointSize;
}

void vtkQtStatisticalBoxChartSeriesOptions::setMarkerSize(const QSizeF &size)
{
  if(size != *this->PointSize)
    {
    *this->PointSize = size;
    emit this->pointMarkerChanged();
    }
}


