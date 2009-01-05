/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartPenBrushGenerator.cxx

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

/// \file vtkQtChartPenBrushGenerator.cxx
/// \date September 22, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartPenBrushGenerator.h"

#include "vtkQtChartColors.h"
#include <QVector>
#include <QtDebug>


class vtkQtChartPenBrushGeneratorInternal
{
public:
  vtkQtChartPenBrushGeneratorInternal();
  ~vtkQtChartPenBrushGeneratorInternal() {}

  QVector<QBrush> Brushes; 
  QVector<QPen> Pens;
};


//-----------------------------------------------------------------------------
vtkQtChartPenBrushGeneratorInternal::vtkQtChartPenBrushGeneratorInternal()
  : Brushes(), Pens()
{
}


//-----------------------------------------------------------------------------
vtkQtChartPenBrushGenerator::vtkQtChartPenBrushGenerator(QObject *parentObject)
  : vtkQtChartStyleGenerator(parentObject)
{
  this->Internal = new vtkQtChartPenBrushGeneratorInternal();
}

vtkQtChartPenBrushGenerator::~vtkQtChartPenBrushGenerator()
{
  delete this->Internal;
}

QBrush vtkQtChartPenBrushGenerator::getSeriesBrush(int index) const
{
  if(this->Internal->Brushes.size() > 0)
    {
    index = index % this->Internal->Brushes.size();
    return this->Internal->Brushes[index];
    }

  return QBrush();
}

QPen vtkQtChartPenBrushGenerator::getSeriesPen(int index) const
{
  if(this->Internal->Pens.size() > 0)
    {
    index = index % this->Internal->Pens.size();
    return this->Internal->Pens[index];
    }

  return QPen();
}

int vtkQtChartPenBrushGenerator::getNumberOfBrushes() const
{
  return this->Internal->Brushes.size();
}

QBrush vtkQtChartPenBrushGenerator::getBrush(int index) const
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

void vtkQtChartPenBrushGenerator::setBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes[index] = color;
    }
}

void vtkQtChartPenBrushGenerator::clearBrushes()
{
  this->Internal->Brushes.clear();
}

void vtkQtChartPenBrushGenerator::addBrushes(const vtkQtChartColors &colors)
{
  for(int i = 0; i < colors.getNumberOfColors(); i++)
    {
    this->Internal->Brushes.append(QBrush(colors.getColor(i)));
    }
}

void vtkQtChartPenBrushGenerator::addBrush(const QBrush &color)
{
  this->Internal->Brushes.append(color);
}

void vtkQtChartPenBrushGenerator::insertBrush(int index, const QBrush &color)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.insert(index, color);
    }
}

void vtkQtChartPenBrushGenerator::removeBrush(int index)
{
  if(index >= 0 && index < this->Internal->Brushes.size())
    {
    this->Internal->Brushes.remove(index);
    }
}

int vtkQtChartPenBrushGenerator::getNumberOfPens() const
{
  return this->Internal->Pens.size();
}

QPen vtkQtChartPenBrushGenerator::getPen(int index) const
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

void vtkQtChartPenBrushGenerator::setPen(int index, const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens[index] = style;
    }
}

void vtkQtChartPenBrushGenerator::clearPens()
{
  this->Internal->Pens.clear();
}

void vtkQtChartPenBrushGenerator::addPens(const vtkQtChartColors &colors)
{
  for(int i = 0; i < colors.getNumberOfColors(); i++)
    {
    this->Internal->Pens.append(QPen(colors.getColor(i)));
    }
}

void vtkQtChartPenBrushGenerator::addPen(const QPen &style)
{
  this->Internal->Pens.append(style);
}

void vtkQtChartPenBrushGenerator::insertPen(int index,
    const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.insert(index, style);
    }
}

void vtkQtChartPenBrushGenerator::removePen(int index)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.remove(index);
    }
}


