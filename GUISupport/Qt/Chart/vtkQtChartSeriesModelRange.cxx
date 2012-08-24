/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesModelRange.cxx

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

/// \file vtkQtChartSeriesModelRange.cxx
/// \date February 19, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesModelRange.h"

#include "vtkMath.h"
#include "vtkQtChartSeriesModel.h"
#include <QDate>
#include <QDateTime>
#include <QTime>

#include <math.h>

vtkQtChartSeriesModelRange::vtkQtChartSeriesModelRange(QObject *parentObject)
  : QObject(parentObject)
{
  this->Model = 0;
  this->XRangeShared = false;
}

void vtkQtChartSeriesModelRange::setModel(vtkQtChartSeriesModel *model,
    bool xShared)
{
  if(this->Model != model)
    {
    if(this->Model)
      {
      this->disconnect(this->Model, 0, this, 0);
      }

    this->Model = model;
    if(this->Model)
      {
      // Use the series change signals to update the ranges.
      this->connect(this->Model, SIGNAL(modelReset()),
          this, SLOT(resetSeries()));
      this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
          this, SLOT(insertSeries(int, int)));
      this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
          this, SLOT(removeSeries(int, int)));
      }

    this->XRangeShared = xShared;
    this->resetSeries();
    }
  else if(this->XRangeShared != xShared)
    {
    this->XRangeShared = xShared;
    this->resetSeries();
    }
}

QList<QVariant> vtkQtChartSeriesModelRange::getSeriesRange(int series,
    int component) const
{
  if(series >= 0 && series < this->Range[1].size())
    {
    if(component == 0 && this->XRangeShared)
      {
      series = 0;
      }

    return this->Range[component][series];
    }

  return QList<QVariant>();
}

void vtkQtChartSeriesModelRange::resetSeries()
{
  // Clean up the range information.
  this->Range[0].clear();
  this->Range[1].clear();

  // Add the new model series.
  if(this->Model)
    {
    int total = this->Model->getNumberOfSeries();
    if(total > 0)
      {
      this->insertSeries(0, total - 1);
      }
    }
}

void vtkQtChartSeriesModelRange::insertSeries(int first, int last)
{
  if(this->Model)
    {
    // Add range entries for the series.
    if(this->XRangeShared && this->Range[1].size() == 0)
      {
      this->Range[0].append(this->computeSeriesRange(0, 0));
      }

    for( ; first <= last; first++)
      {
      this->Range[1].insert(first, this->computeSeriesRange(first, 1));
      if(!this->XRangeShared)
        {
        this->Range[0].insert(first, this->computeSeriesRange(first, 0));
        }
      }
    }
}

void vtkQtChartSeriesModelRange::removeSeries(int first, int last)
{
  // Remove range entries for the series.
  for( ; last >= first; last--)
    {
    this->Range[1].removeAt(last);
    if(!this->XRangeShared)
      {
      this->Range[0].removeAt(last);
      }
    }

  if(this->XRangeShared && this->Range[1].size() == 0)
    {
    this->Range[0].clear();
    }
}

QList<QVariant> vtkQtChartSeriesModelRange::computeSeriesRange(int series,
    int component)
{
  QList<QVariant> range;
  if(this->Model)
    {
    int total = this->Model->getNumberOfSeriesValues(series);
    int i;
    QVariant value;
    QVariant::Type valueType = QVariant::Invalid;

    // Find the first non-NULL, non-NaN value.  Use it to determine type
    // and initialize min/max values.
    for (i = 0; i < total; i++)
      {
      value = this->Model->getSeriesValue(series, i, component);
      valueType = value.type();
      // Check to see if the the value is invalid and we have to continue loop.
      if (value.isNull()) continue;
      if (!value.isValid()) continue;
      if ((valueType == QVariant::Double) && vtkMath::IsNan(value.toDouble())) continue;
      // If we got here, we passed all the checks.  Break out.
      break;
      }

    // If we found a valid entry that is not a string (for which range has no
    // meaning), then continue to compute the range.
    if ((i < total) && (valueType != QVariant::String))
      {
      range.append(value);
      range.append(value);
      // Continue iteration over values.
      for(i++; i < total; i++)
        {
        value = this->Model->getSeriesValue(series, i, component);
        if(value.type() != valueType)
          {
          continue;
          }

        if(valueType == QVariant::Int)
          {
          range[0] = qMin<int>(value.toInt(), range[0].toInt());
          range[1] = qMax<int>(value.toInt(), range[1].toInt());
          }
        else if(valueType == QVariant::Double)
          {
          if (!vtkMath::IsNan(value.toDouble()))
            {
            range[0] = qMin<double>(value.toDouble(), range[0].toDouble());
            range[1] = qMax<double>(value.toDouble(), range[1].toDouble());
            }
          }
        else if(valueType == QVariant::Date)
          {
          range[0] = qMin<QDate>(value.toDate(), range[0].toDate());
          range[1] = qMax<QDate>(value.toDate(), range[1].toDate());
          }
        else if(valueType == QVariant::DateTime)
          {
          range[0] = qMin<QDateTime>(value.toDateTime(),
              range[0].toDateTime());
          range[1] = qMax<QDateTime>(value.toDateTime(),
              range[1].toDateTime());
          }
        else if(valueType == QVariant::Time)
          {
          range[0] = qMin<QTime>(value.toTime(), range[0].toTime());
          range[1] = qMax<QTime>(value.toTime(), range[1].toTime());
          }
        }
      }
    }

  return range;
}


