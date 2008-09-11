/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleGenerator.cxx

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

/// \file vtkQtChartStyleGenerator.cxx
/// \date February 15, 2008

#include "vtkQtChartStyleGenerator.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QVector>
#include <QDebug>

class vtkQtChartStyleGeneratorInternal
{
public:
  vtkQtChartStyleGeneratorInternal();
  ~vtkQtChartStyleGeneratorInternal() {}

  QVector<QBrush> Brushes; 
  QVector<QPen> Pens;
};


//----------------------------------------------------------------------------
vtkQtChartStyleGeneratorInternal::vtkQtChartStyleGeneratorInternal()
  : Brushes(), Pens()
{
}


//----------------------------------------------------------------------------
vtkQtChartStyleGenerator::vtkQtChartStyleGenerator(
    QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartStyleGeneratorInternal();
}

vtkQtChartStyleGenerator::~vtkQtChartStyleGenerator()
{
  delete this->Internal;
}

int vtkQtChartStyleGenerator::getNumberOfBrushes() const
{
  return this->Internal->Brushes.size();
}

int vtkQtChartStyleGenerator::getNumberOfPens() const
{
  return this->Internal->Pens.size();
}

QBrush vtkQtChartStyleGenerator::getBrush(int index) const
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    return this->Internal->Brushes[index];
    }
  else
    {
    qWarning() << "ERROR: Cannot return brush with index "
               << index << " since only "
               << this->getNumberOfBrushes() << " are available";
    return QBrush();
    }
}

QPen vtkQtChartStyleGenerator::getPen(int index) const
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    return this->Internal->Pens[index];
    }
    {
    qWarning() << "ERROR: Cannot return pen with index "
               << index << " since only "
               << this->getNumberOfPens() << " are available";
    return QPen();
    }
}

QBrush vtkQtChartStyleGenerator::getSeriesBrush(int index) const
{
  if(this->Internal->Brushes.size() > 0)
    {
    index = index % this->Internal->Brushes.size();
    return this->Internal->Brushes[index];
    }
  return QBrush();
}

QPen vtkQtChartStyleGenerator::getSeriesPen(int index) const
{
  if(this->Internal->Pens.size() > 0)
    {
    index = index % this->Internal->Pens.size();
    return this->Internal->Pens[index];
    }
  else
    {
    return QPen();
    }
}

void vtkQtChartStyleGenerator::clearBrushes()
{
  this->Internal->Brushes.clear();
}

void vtkQtChartStyleGenerator::addBrush(const QBrush &color)
{
  this->Internal->Brushes.append(color);
}

void vtkQtChartStyleGenerator::insertBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.insert(index, color);
    }
}

void vtkQtChartStyleGenerator::setBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes[index] = color;
    }
}

void vtkQtChartStyleGenerator::removeBrush(int index)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.remove(index);
    }
}

void vtkQtChartStyleGenerator::clearPens()
{
  this->Internal->Pens.clear();
}

void vtkQtChartStyleGenerator::addPen(const QPen &style)
{
  this->Internal->Pens.append(style);
}

void vtkQtChartStyleGenerator::insertPen(int index,
    const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.insert(index, style);
    }
}

void vtkQtChartStyleGenerator::setPen(int index, const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens[index] = style;
    }
}

void vtkQtChartStyleGenerator::removePen(int index)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.remove(index);
    }
}


