/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelection.cxx

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

/// \file vtkQtChartSeriesSelection.cxx
/// \date March 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesSelection.h"

#include <QList>


class vtkQtChartSeriesSelectionInternal
{
public:
  vtkQtChartSeriesSelectionInternal();
  vtkQtChartSeriesSelectionInternal(const vtkQtChartIndexRangeList &series,
      const QMap<int, vtkQtChartIndexRangeList> &points);
  ~vtkQtChartSeriesSelectionInternal() {}

  vtkQtChartIndexRangeList Series;
  QMap<int, vtkQtChartIndexRangeList> Points;
};


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelectionInternal::vtkQtChartSeriesSelectionInternal()
  : Series(), Points()
{
}

vtkQtChartSeriesSelectionInternal::vtkQtChartSeriesSelectionInternal(
    const vtkQtChartIndexRangeList &series,
    const QMap<int, vtkQtChartIndexRangeList> &points)
  : Series(series), Points(points)
{
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelection::vtkQtChartSeriesSelection()
{
  this->Internal = new vtkQtChartSeriesSelectionInternal();
}

vtkQtChartSeriesSelection::vtkQtChartSeriesSelection(
    const vtkQtChartSeriesSelection &other)
{
  this->Internal = new vtkQtChartSeriesSelectionInternal(
      other.Internal->Series, other.Internal->Points);
}

vtkQtChartSeriesSelection::~vtkQtChartSeriesSelection()
{
  delete this->Internal;
}

vtkQtChartSeriesSelection &vtkQtChartSeriesSelection::operator=(
    const vtkQtChartSeriesSelection &other)
{
  this->Internal->Series = other.Internal->Series;
  this->Internal->Points = other.Internal->Points;
  return *this;
}

bool vtkQtChartSeriesSelection::isEmpty() const
{
  return this->Internal->Series.isEmpty() && this->Internal->Points.isEmpty();
}

vtkQtChartSeriesSelection::SelectionType
vtkQtChartSeriesSelection::getType() const
{
  if(!this->Internal->Series.isEmpty())
    {
    return vtkQtChartSeriesSelection::SeriesSelection;
    }
  else if(!this->Internal->Points.isEmpty())
    {
    return vtkQtChartSeriesSelection::PointSelection;
    }

  return vtkQtChartSeriesSelection::NoSelection;
}

bool vtkQtChartSeriesSelection::clear()
{
  bool changed = !this->Internal->Series.isEmpty() ||
      !this->Internal->Points.isEmpty();
  this->Internal->Series.clear();
  this->Internal->Points.clear();
  return changed;
}

const vtkQtChartIndexRangeList &vtkQtChartSeriesSelection::getSeries() const
{
  return this->Internal->Series;
}

bool vtkQtChartSeriesSelection::setSeries(
    const vtkQtChartIndexRangeList &series)
{
  bool changed = this->clear();
  if(this->Internal->Series.setRanges(series))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::setSeries(int first, int second)
{
  bool changed = this->clear();
  if(this->Internal->Series.setRange(first, second))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::addSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.addRanges(series);
    }

  return false;
}

bool vtkQtChartSeriesSelection::addSeries(int first, int last)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.addRange(first, last);
    }

  return false;
}

bool vtkQtChartSeriesSelection::subtractSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.subtractRanges(series);
    }

  return false;
}

bool vtkQtChartSeriesSelection::subtractSeries(int first, int last)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.subtractRange(first, last);
    }

  return false;
}

bool vtkQtChartSeriesSelection::xorSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.xorRanges(series);
    }

  return false;
}

bool vtkQtChartSeriesSelection::xorSeries(int first, int last)
{
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.xorRange(first, last);
    }

  return false;
}

void vtkQtChartSeriesSelection::limitSeries(int minimum, int maximum)
{
  if(this->Internal->Points.isEmpty())
    {
    this->Internal->Series.limitRange(minimum, maximum);
    }
  else
    {
    QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
        this->Internal->Points.begin();
    while(iter != this->Internal->Points.end())
      {
      if(iter.key() < minimum || iter.key() > maximum)
        {
        iter = this->Internal->Points.erase(iter);
        }
      else
        {
        ++iter;
        }
      }
    }
}

bool vtkQtChartSeriesSelection::offsetSeries(int first, int offset)
{
  bool changed = false;
  if(this->Internal->Points.isEmpty())
    {
    return this->Internal->Series.offsetRanges(first, offset);
    }
  else
    {
    // The keys are returned in ascending order.
    QList<int> series = this->Internal->Points.keys();
    if(offset > 0)
      {
      // Loop backwards through the list of keys to change them.
      for(int i = series.size() - 1; i >= 0; i--)
        {
        if(series[i] >= first)
          {
          vtkQtChartIndexRangeList list =
              this->Internal->Points.take(series[i]);
          this->Internal->Points.insert(series[i] + offset, list);
          changed = true;
          }
        else
          {
          // Skip the items before the first.
          break;
          }
        }
      }
    else
      {
      // Loop forward through the list of keys to change them.
      for(int i = 0; i < series.size(); i++)
        {
        if(series[i] >= first)
          {
          vtkQtChartIndexRangeList list =
              this->Internal->Points.take(series[i]);
          this->Internal->Points.insert(series[i] + offset, list);
          changed = true;
          }
        }
      }
    }

  return changed;
}

const QMap<int, vtkQtChartIndexRangeList> &
vtkQtChartSeriesSelection::getPoints() const
{
  return this->Internal->Points;
}

bool vtkQtChartSeriesSelection::setPoints(
    const QMap<int, vtkQtChartIndexRangeList> &points)
{
  bool changed = this->clear();
  if(this->addPoints(points))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::setPoints(int series,
    const vtkQtChartIndexRangeList &indexes)
{
  bool changed = this->clear();
  if(this->addPoints(series, indexes))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::addPoints(
    const QMap<int, vtkQtChartIndexRangeList> &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::ConstIterator iter = points.begin();
  for( ; iter != points.end(); ++iter)
    {
    if(this->addPoints(iter.key(), *iter))
      {
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::addPoints(int series,
    const vtkQtChartIndexRangeList &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
      this->Internal->Points.find(series);
  if(iter == this->Internal->Points.end())
    {
    this->Internal->Points.insert(series, points);
    changed = true;
    }
  else
    {
    changed = iter->addRanges(points);
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractPoints(
    const QMap<int, vtkQtChartIndexRangeList> &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty() ||
      this->Internal->Points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::ConstIterator iter = points.begin();
  for( ; iter != points.end(); ++iter)
    {
    if(this->subtractPoints(iter.key(), *iter))
      {
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractPoints(int series,
    const vtkQtChartIndexRangeList &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty() ||
      this->Internal->Points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
      this->Internal->Points.find(series);
  if(iter != this->Internal->Points.end())
    {
    changed = iter->subtractRanges(points);
    if(iter->isEmpty())
      {
      this->Internal->Points.erase(iter);
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractPoints(int first, int last)
{
  if(!this->Internal->Series.isEmpty() || this->Internal->Points.isEmpty())
    {
    return false;
    }

  // Remove the series range along with any series points.
  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
      this->Internal->Points.begin();
  while(iter != this->Internal->Points.end())
    {
    if(iter.key() >= first || iter.key() <= last)
      {
      iter = this->Internal->Points.erase(iter);
      changed = true;
      }
    else
      {
      ++iter;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::xorPoints(
    const QMap<int, vtkQtChartIndexRangeList> &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::ConstIterator iter = points.begin();
  for( ; iter != points.end(); ++iter)
    {
    if(this->xorPoints(iter.key(), *iter))
      {
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::xorPoints(int series,
    const vtkQtChartIndexRangeList &points)
{
  if(!this->Internal->Series.isEmpty() || points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
      this->Internal->Points.find(series);
  if(iter == this->Internal->Points.end())
    {
    this->Internal->Points.insert(series, points);
    changed = true;
    }
  else
    {
    changed = iter->xorRanges(points);
    if(iter->isEmpty())
      {
      this->Internal->Points.erase(iter);
      }
    }

  return changed;
}

void vtkQtChartSeriesSelection::limitPoints(int series, int minimum,
    int maximum)
{
  QMap<int, vtkQtChartIndexRangeList>::Iterator iter =
      this->Internal->Points.find(series);
  if(iter != this->Internal->Points.end())
    {
    iter->limitRange(minimum, maximum);
    if(iter->isEmpty())
      {
      this->Internal->Points.erase(iter);
      }
    }
}


