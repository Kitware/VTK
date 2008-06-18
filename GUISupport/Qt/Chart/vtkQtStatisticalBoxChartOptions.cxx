/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartOptions.cxx

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

/// \file vtkQtStatisticalBoxChartOptions.cxx
/// \date May 15, 2008

#include "vtkQtStatisticalBoxChartOptions.h"


const QColor vtkQtStatisticalBoxChartOptions::LightBlue = QColor(125, 165, 230);

vtkQtStatisticalBoxChartOptions::vtkQtStatisticalBoxChartOptions(QObject *parentObject)
  : QObject(parentObject), Highlight(vtkQtStatisticalBoxChartOptions::LightBlue)
{
  this->AxesCorner = vtkQtChartLayer::BottomLeft;
  this->OutlineType = vtkQtStatisticalBoxChartOptions::Darker;
  this->GroupFraction = (float)0.7;
    this->BarFraction = (float)0.8;
  //  this->BarFraction = (float)0.4;
}

vtkQtStatisticalBoxChartOptions::vtkQtStatisticalBoxChartOptions(const vtkQtStatisticalBoxChartOptions &other)
  : QObject(), Highlight(other.Highlight)
{
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->GroupFraction = other.GroupFraction;
  this->BarFraction = other.BarFraction;
}

void vtkQtStatisticalBoxChartOptions::setAxesCorner(vtkQtChartLayer::AxesCorner axes)
{
  if(this->AxesCorner != axes)
    {
    this->AxesCorner = axes;
    emit this->axesCornerChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setBarGroupFraction(float fraction)
{
  if(this->GroupFraction != fraction)
    {
    this->GroupFraction = fraction;
    emit this->barFractionsChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setBarWidthFraction(float fraction)
{
  if(this->BarFraction != fraction)
    {
    this->BarFraction = fraction;
    emit this->barFractionsChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setBinOutlineStyle(
    vtkQtStatisticalBoxChartOptions::OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit this->outlineStyleChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setHighlightColor(const QColor &color)
{
  if(this->Highlight != color)
    {
    this->Highlight = color;
    emit this->highlightChanged();
    }
}

vtkQtStatisticalBoxChartOptions &vtkQtStatisticalBoxChartOptions::operator=(
    const vtkQtStatisticalBoxChartOptions &other)
{
  this->Highlight = other.Highlight;
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->GroupFraction = other.GroupFraction;
  this->BarFraction = other.BarFraction;
  return *this;
}


