/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesHueRange.cxx

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

/// \file vtkQtChartSeriesHueRange.cxx
/// \date February 26, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesHueRange.h"

#include "vtkQtChartColors.h"
#include "vtkQtChartSeriesModel.h"

#include <QBrush>
#include <QColor>
#include <QList>


class vtkQtChartSeriesHueRangeItem
{
public:
  vtkQtChartSeriesHueRangeItem();
  vtkQtChartSeriesHueRangeItem(const QColor &first, const QColor &second);
  vtkQtChartSeriesHueRangeItem(const vtkQtChartSeriesHueRangeItem &other);
  ~vtkQtChartSeriesHueRangeItem() {}

  vtkQtChartSeriesHueRangeItem &operator=(
      const vtkQtChartSeriesHueRangeItem &other);

  QColor getColor(int index, int total) const;

  QColor First;
  QColor Second;
};


class vtkQtChartSeriesHueRangeInternal
{
public:
  vtkQtChartSeriesHueRangeInternal();
  ~vtkQtChartSeriesHueRangeInternal() {}

  QList<vtkQtChartSeriesHueRangeItem> Ranges;
};


//-----------------------------------------------------------------------------
vtkQtChartSeriesHueRangeItem::vtkQtChartSeriesHueRangeItem()
  : First(), Second()
{
}

vtkQtChartSeriesHueRangeItem::vtkQtChartSeriesHueRangeItem(const QColor &first,
    const QColor &second)
  : First(first), Second(second)
{
}

vtkQtChartSeriesHueRangeItem::vtkQtChartSeriesHueRangeItem(
    const vtkQtChartSeriesHueRangeItem &other)
  : First(other.First), Second(other.Second)
{
}

vtkQtChartSeriesHueRangeItem &vtkQtChartSeriesHueRangeItem::operator=(
    const vtkQtChartSeriesHueRangeItem &other)
{
  this->First = other.First;
  this->Second = other.Second;
  return *this;
}

QColor vtkQtChartSeriesHueRangeItem::getColor(int index, int total) const
{
  // Interpolate the HSV values.
  float fraction = (float)index / (float)total;
  return vtkQtChartColors::interpolateHsv(this->First, this->Second, fraction);
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesHueRangeInternal::vtkQtChartSeriesHueRangeInternal()
  : Ranges()
{
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesHueRange::vtkQtChartSeriesHueRange(QObject *parentObject)
  : vtkQtChartSeriesColors(parentObject)
{
  this->Internal = new vtkQtChartSeriesHueRangeInternal();
}

vtkQtChartSeriesHueRange::~vtkQtChartSeriesHueRange()
{
  delete this->Internal;
}

void vtkQtChartSeriesHueRange::getBrush(int series, int index,
    QBrush &brush) const
{
  vtkQtChartSeriesModel *model = this->getModel();
  if(model && series >= 0 && series < this->Internal->Ranges.size() &&
      series < model->getNumberOfSeries())
    {
    int total = model->getNumberOfSeriesValues(series);
    brush.setColor(this->Internal->Ranges[series].getColor(index, total));
    }
}

int vtkQtChartSeriesHueRange::getNumberOfRanges() const
{
  return this->Internal->Ranges.size();
}

void vtkQtChartSeriesHueRange::addRange(const QColor &color1,
    const QColor &color2)
{
  this->Internal->Ranges.append(vtkQtChartSeriesHueRangeItem(color1, color2));
}

void vtkQtChartSeriesHueRange::removeRange(int index)
{
  if(index >= 0 && index < this->Internal->Ranges.size())
    {
    this->Internal->Ranges.removeAt(index);
    }
}

void vtkQtChartSeriesHueRange::removeAllRanges()
{
  this->Internal->Ranges.clear();
}


