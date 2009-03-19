/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartOptions.cxx

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

/// \file vtkQtBarChartOptions.cxx
/// \date February 22, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtBarChartOptions.h"

#include "vtkQtChartHelpFormatter.h"


//const QColor vtkQtBarChartOptions::LightBlue = QColor(125, 165, 230);

vtkQtBarChartOptions::vtkQtBarChartOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->AxesCorner = vtkQtChartLayer::BottomLeft;
  this->OutlineType = vtkQtBarChartOptions::Darker;
  this->Help = new vtkQtChartHelpFormatter("%s: %1, %2");
  this->GroupFraction = (float)0.7;
  this->BarFraction = (float)0.8;
}

vtkQtBarChartOptions::vtkQtBarChartOptions(const vtkQtBarChartOptions &other)
  : QObject()
{
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->Help = new vtkQtChartHelpFormatter(other.Help->getFormat());
  this->GroupFraction = other.GroupFraction;
  this->BarFraction = other.BarFraction;
}

vtkQtBarChartOptions::~vtkQtBarChartOptions()
{
  delete this->Help;
}

void vtkQtBarChartOptions::setAxesCorner(vtkQtChartLayer::AxesCorner axes)
{
  if(this->AxesCorner != axes)
    {
    this->AxesCorner = axes;
    emit this->axesCornerChanged();
    }
}

void vtkQtBarChartOptions::setBarGroupFraction(float fraction)
{
  if(this->GroupFraction != fraction)
    {
    this->GroupFraction = fraction;
    emit this->barFractionsChanged();
    }
}

void vtkQtBarChartOptions::setBarWidthFraction(float fraction)
{
  if(this->BarFraction != fraction)
    {
    this->BarFraction = fraction;
    emit this->barFractionsChanged();
    }
}

void vtkQtBarChartOptions::setOutlineStyle(
    vtkQtBarChartOptions::OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit this->outlineStyleChanged();
    }
}

vtkQtBarChartOptions &vtkQtBarChartOptions::operator=(
    const vtkQtBarChartOptions &other)
{
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->Help->setFormat(other.Help->getFormat());
  this->GroupFraction = other.GroupFraction;
  this->BarFraction = other.BarFraction;
  return *this;
}


