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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtStatisticalBoxChartOptions.h"

#include "vtkQtChartHelpFormatter.h"


vtkQtStatisticalBoxChartOptions::vtkQtStatisticalBoxChartOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->AxesCorner = vtkQtChartLayer::BottomLeft;
  this->OutlineType = vtkQtStatisticalBoxChartOptions::Darker;
  this->Help = new vtkQtChartHelpFormatter(
    "%s\nLower Quartile: %1\nMedian: %2\nUpper Quartile: %3");
  this->Outlier = new vtkQtChartHelpFormatter("%s, %1");
  this->BoxFraction = (float)0.8;
  //this->BoxFraction = (float)0.4;
}

vtkQtStatisticalBoxChartOptions::vtkQtStatisticalBoxChartOptions(const vtkQtStatisticalBoxChartOptions &other)
  : QObject()
{
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->Help = new vtkQtChartHelpFormatter(other.Help->getFormat());
  this->Outlier = new vtkQtChartHelpFormatter(other.Outlier->getFormat());
  this->BoxFraction = other.BoxFraction;
}

vtkQtStatisticalBoxChartOptions::~vtkQtStatisticalBoxChartOptions()
{
  delete this->Help;
  delete this->Outlier;
}

void vtkQtStatisticalBoxChartOptions::setAxesCorner(vtkQtChartLayer::AxesCorner axes)
{
  if(this->AxesCorner != axes)
    {
    this->AxesCorner = axes;
    emit this->axesCornerChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setBoxWidthFraction(float fraction)
{
  if(this->BoxFraction != fraction)
    {
    this->BoxFraction = fraction;
    emit this->boxFractionChanged();
    }
}

void vtkQtStatisticalBoxChartOptions::setOutlineStyle(
    vtkQtStatisticalBoxChartOptions::OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit this->outlineStyleChanged();
    }
}

vtkQtStatisticalBoxChartOptions &vtkQtStatisticalBoxChartOptions::operator=(
    const vtkQtStatisticalBoxChartOptions &other)
{
  this->AxesCorner = other.AxesCorner;
  this->OutlineType = other.OutlineType;
  this->Help->setFormat(other.Help->getFormat());
  this->Outlier->setFormat(other.Outlier->getFormat());
  this->BoxFraction = other.BoxFraction;
  return *this;
}


