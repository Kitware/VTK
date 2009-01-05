/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChartOptions.cxx

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

/// \file vtkQtStackedChartOptions.cxx
/// \date February 27, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtStackedChartOptions.h"

#include "vtkQtChartHelpFormatter.h"


vtkQtStackedChartOptions::vtkQtStackedChartOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Axes = vtkQtChartLayer::BottomLeft;
  this->Help = new vtkQtChartHelpFormatter("%s: %1, %3");
  this->Normalized = false;
  this->Gradient = false;
}

vtkQtStackedChartOptions::vtkQtStackedChartOptions(
    const vtkQtStackedChartOptions &other)
  : QObject()
{
  this->Axes = other.Axes;
  this->Help = new vtkQtChartHelpFormatter(other.Help->getFormat());
  this->Normalized = other.Normalized;
  this->Gradient = other.Gradient;
}

vtkQtStackedChartOptions::~vtkQtStackedChartOptions()
{
  delete this->Help;
}

void vtkQtStackedChartOptions::setAxesCorner(vtkQtChartLayer::AxesCorner axes)
{
  if(this->Axes != axes)
    {
    this->Axes = axes;
    emit this->axesCornerChanged();
    }
}

void vtkQtStackedChartOptions::setSumNormalized(bool normalized)
{
  if(this->Normalized != normalized)
    {
    this->Normalized = normalized;
    emit this->sumationChanged();
    }
}

void vtkQtStackedChartOptions::setGradientDisplayed(bool gradient)
{
  if(this->Gradient != gradient)
    {
    this->Gradient = gradient;
    emit this->gradientChanged();
    }
}

vtkQtStackedChartOptions &vtkQtStackedChartOptions::operator=(
    const vtkQtStackedChartOptions &other)
{
  this->Axes = other.Axes;
  this->Help->setFormat(other.Help->getFormat());
  this->Normalized = other.Normalized;
  this->Gradient = other.Gradient;
  return *this;
}


