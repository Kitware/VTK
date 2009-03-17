/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartPenGenerator.cxx

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

/// \file vtkQtChartPenGenerator.cxx
/// \date March 17, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartPenGenerator.h"

#include "vtkQtChartColors.h"
#include <QVector>
#include <QtDebug>


class vtkQtChartPenGeneratorInternal
{
public:
  vtkQtChartPenGeneratorInternal();
  ~vtkQtChartPenGeneratorInternal() {}

  QVector<QPen> Pens;
};


//-----------------------------------------------------------------------------
vtkQtChartPenGeneratorInternal::vtkQtChartPenGeneratorInternal()
  : Pens()
{
}


//-----------------------------------------------------------------------------
vtkQtChartPenGenerator::vtkQtChartPenGenerator(QObject *parentObject)
  : vtkQtChartStylePen(parentObject)
{
  this->Internal = new vtkQtChartPenGeneratorInternal();
}

vtkQtChartPenGenerator::~vtkQtChartPenGenerator()
{
  delete this->Internal;
}

QPen vtkQtChartPenGenerator::getStylePen(int index) const
{
  if(index >= 0 && this->Internal->Pens.size() > 0)
    {
    index = index % this->Internal->Pens.size();
    return this->Internal->Pens[index];
    }

  return QPen();
}

int vtkQtChartPenGenerator::getNumberOfPens() const
{
  return this->Internal->Pens.size();
}

QPen vtkQtChartPenGenerator::getPen(int index) const
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    return this->Internal->Pens[index];
    }
  else
    {
    qWarning() << "Error: Pen index out of range.";
    return QPen();
    }
}

void vtkQtChartPenGenerator::setPen(int index, const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens[index] = style;
    }
}

void vtkQtChartPenGenerator::clearPens()
{
  this->Internal->Pens.clear();
}

void vtkQtChartPenGenerator::addPens(const vtkQtChartColors &colors)
{
  for(int i = 0; i < colors.getNumberOfColors(); i++)
    {
    this->Internal->Pens.append(QPen(colors.getColor(i)));
    }
}

void vtkQtChartPenGenerator::addPen(const QPen &style)
{
  this->Internal->Pens.append(style);
}

void vtkQtChartPenGenerator::insertPen(int index, const QPen &style)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.insert(index, style);
    }
}

void vtkQtChartPenGenerator::removePen(int index)
{
  if(index >= 0 && index < this->Internal->Pens.size())
    {
    this->Internal->Pens.remove(index);
    }
}


