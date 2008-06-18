/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptions.cxx

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

/// \file vtkQtChartSeriesOptions.cxx
/// \date February 15, 2008

#include "vtkQtChartSeriesOptions.h"

#include "vtkQtChartStyleGenerator.h"
#include <QBrush>
#include <QPen>


vtkQtChartSeriesOptions::vtkQtChartSeriesOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Pen = new QPen(Qt::black);
  this->Brush = new QBrush();
  this->Style = -1;
  this->Visible = true;
}

vtkQtChartSeriesOptions::~vtkQtChartSeriesOptions()
{
  delete this->Pen;
  delete this->Brush;
}

void vtkQtChartSeriesOptions::setStyle(int style, vtkQtChartStyleGenerator *)
{
  this->Style = style;
}

void vtkQtChartSeriesOptions::setVisible(bool visible)
{
  if(this->Visible != visible)
    {
    this->Visible = visible;
    emit this->visibilityChanged(visible);
    }
}

const QPen &vtkQtChartSeriesOptions::getPen() const
{
  return *this->Pen;
}

void vtkQtChartSeriesOptions::setPen(const QPen &pen)
{
  if(*this->Pen != pen)
    {
    *this->Pen = pen;
    emit this->penChanged(pen);
    }
}

const QBrush &vtkQtChartSeriesOptions::getBrush() const
{
  return *this->Brush;
}

void vtkQtChartSeriesOptions::setBrush(const QBrush &brush)
{
  if(*this->Brush != brush)
    {
    *this->Brush = brush;
    emit this->brushChanged(brush);
    }
}

