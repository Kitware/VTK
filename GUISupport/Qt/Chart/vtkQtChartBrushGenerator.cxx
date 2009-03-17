/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBrushGenerator.cxx

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

/// \file vtkQtChartBrushGenerator.cxx
/// \date March 17, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartBrushGenerator.h"

#include "vtkQtChartColors.h"
#include <QVector>
#include <QtDebug>


class vtkQtChartBrushGeneratorInternal
{
public:
  vtkQtChartBrushGeneratorInternal();
  ~vtkQtChartBrushGeneratorInternal() {}

  QVector<QBrush> Brushes; 
};


//-----------------------------------------------------------------------------
vtkQtChartBrushGeneratorInternal::vtkQtChartBrushGeneratorInternal()
  : Brushes()
{
}


//-----------------------------------------------------------------------------
vtkQtChartBrushGenerator::vtkQtChartBrushGenerator(QObject *parentObject)
  : vtkQtChartStyleBrush(parentObject)
{
  this->Internal = new vtkQtChartBrushGeneratorInternal();
}

vtkQtChartBrushGenerator::~vtkQtChartBrushGenerator()
{
  delete this->Internal;
}

QBrush vtkQtChartBrushGenerator::getStyleBrush(int index) const
{
  if(index >= 0 && this->Internal->Brushes.size() > 0)
    {
    index = index % this->Internal->Brushes.size();
    return this->Internal->Brushes[index];
    }

  return QBrush();
}

int vtkQtChartBrushGenerator::getNumberOfBrushes() const
{
  return this->Internal->Brushes.size();
}

QBrush vtkQtChartBrushGenerator::getBrush(int index) const
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    return this->Internal->Brushes[index];
    }
  else
    {
    qWarning() << "Error: Brush index out of range.";
    return QBrush();
    }
}

void vtkQtChartBrushGenerator::setBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes[index] = color;
    }
}

void vtkQtChartBrushGenerator::clearBrushes()
{
  this->Internal->Brushes.clear();
}

void vtkQtChartBrushGenerator::addBrushes(const vtkQtChartColors &colors)
{
  for(int i = 0; i < colors.getNumberOfColors(); i++)
    {
    this->Internal->Brushes.append(QBrush(colors.getColor(i)));
    }
}

void vtkQtChartBrushGenerator::addBrush(const QBrush &color)
{
  this->Internal->Brushes.append(color);
}

void vtkQtChartBrushGenerator::insertBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.insert(index, color);
    }
}

void vtkQtChartBrushGenerator::removeBrush(int index)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.remove(index);
    }
}


