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

#include "vtkQtChartSeriesSelection.h"


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelectionItem::vtkQtChartSeriesSelectionItem()
  : Points()
{
  this->Series = -1;
}

vtkQtChartSeriesSelectionItem::vtkQtChartSeriesSelectionItem(int series)
  : Points()
{
  this->Series = series;
}

vtkQtChartSeriesSelectionItem::vtkQtChartSeriesSelectionItem(
    const vtkQtChartSeriesSelectionItem &other)
  : Points(other.Points)
{
  this->Series = other.Series;
}

vtkQtChartSeriesSelectionItem &vtkQtChartSeriesSelectionItem::operator=(
    const vtkQtChartSeriesSelectionItem &other)
{
  this->Series = other.Series;
  this->Points = other.Points;
  return *this;
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelection::vtkQtChartSeriesSelection()
  : Series(), Points()
{
}

vtkQtChartSeriesSelection::vtkQtChartSeriesSelection(
    const vtkQtChartSeriesSelection &other)
  : Series(other.Series), Points(other.Points)
{
}

bool vtkQtChartSeriesSelection::isEmpty() const
{
  return this->Series.isEmpty() && this->Points.isEmpty();
}

vtkQtChartSeriesSelection::SelectionType
vtkQtChartSeriesSelection::getType() const
{
  if(this->Series.size() > 0)
    {
    return vtkQtChartSeriesSelection::SeriesSelection;
    }
  else if(this->Points.size() > 0)
    {
    return vtkQtChartSeriesSelection::PointSelection;
    }

  return vtkQtChartSeriesSelection::NoSelection;
}

bool vtkQtChartSeriesSelection::clear()
{
  bool changed = this->Series.size() > 0 || this->Points.size() > 0;
  this->Series.clear();
  this->Points.clear();
  return changed;
}

const vtkQtChartIndexRangeList &vtkQtChartSeriesSelection::getSeries() const
{
  return this->Series;
}

bool vtkQtChartSeriesSelection::setSeries(
    const vtkQtChartIndexRangeList &series)
{
  bool changed = this->clear();
  if(this->addSeries(series))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::setSeries(const vtkQtChartIndexRange &series)
{
  bool changed = this->clear();
  if(this->addSeries(series))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::addSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Points.isEmpty())
    {
    return this->addRanges(series, this->Series);
    }

  return false;
}

bool vtkQtChartSeriesSelection::addSeries(const vtkQtChartIndexRange &series)
{
  vtkQtChartIndexRangeList list;
  list.append(series);
  return this->addSeries(list);
}

bool vtkQtChartSeriesSelection::subtractSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Points.isEmpty())
    {
    return this->subtractRanges(series, this->Series);
    }

  return false;
}

bool vtkQtChartSeriesSelection::subtractSeries(
    const vtkQtChartIndexRange &series)
{
  vtkQtChartIndexRangeList list;
  list.append(series);
  return this->subtractSeries(list);
}

bool vtkQtChartSeriesSelection::xorSeries(
    const vtkQtChartIndexRangeList &series)
{
  if(this->Points.isEmpty() && !series.isEmpty())
    {
    if(this->Series.isEmpty())
      {
      return this->addRanges(series, this->Series);
      }
    else
      {
      vtkQtChartIndexRangeList temp = series;
      this->subtractRanges(this->Series, temp);
      this->subtractRanges(series, this->Series);
      this->addRanges(temp, this->Series);
      return true;
      }
    }

  return false;
}

bool vtkQtChartSeriesSelection::xorSeries(const vtkQtChartIndexRange &series)
{
  vtkQtChartIndexRangeList list;
  list.append(series);
  return this->xorSeries(list);
}

void vtkQtChartSeriesSelection::limitSeries(int minimum, int maximum)
{
  if(this->Points.isEmpty())
    {
    this->limitRanges(this->Series, minimum, maximum);
    }
  else
    {
    QList<vtkQtChartSeriesSelectionItem>::Iterator iter = this->Points.begin();
    while(iter != this->Points.end())
      {
      if(iter->Series < minimum || iter->Series > maximum)
        {
        iter = this->Points.erase(iter);
        }
      else
        {
        ++iter;
        }
      }
    }
}

const QList<vtkQtChartSeriesSelectionItem> &
vtkQtChartSeriesSelection::getPoints() const
{
  return this->Points;
}

bool vtkQtChartSeriesSelection::setPoints(
    const QList<vtkQtChartSeriesSelectionItem> &points)
{
  bool changed = this->clear();
  if(this->addPoints(points))
    {
    changed = true;
    }

  return changed;
}

bool vtkQtChartSeriesSelection::addPoints(
    const QList<vtkQtChartSeriesSelectionItem> &points)
{
  if(!this->Series.isEmpty() || points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter = points.begin();
  for( ; iter != points.end(); ++iter)
    {
    if(iter->Series < 0 || iter->Points.isEmpty())
      {
      continue;
      }

    bool doAdd = true;
    QList<vtkQtChartSeriesSelectionItem>::Iterator jter = this->Points.begin();
    for( ; jter != this->Points.end(); ++jter)
      {
      if(iter->Series < jter->Series)
        {
        jter = this->Points.insert(jter,
            vtkQtChartSeriesSelectionItem(iter->Series));
        this->addRanges(iter->Points, jter->Points);
        changed = true;
        doAdd = false;
        break;
        }
      else if(iter->Series == jter->Series)
        {
        if(this->addRanges(iter->Points, jter->Points))
          {
          changed = true;
          }

        doAdd = false;
        break;
        }
      }

    if(doAdd)
      {
      this->Points.append(vtkQtChartSeriesSelectionItem(iter->Series));
      this->addRanges(iter->Points, this->Points.last().Points);
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractPoints(
    const QList<vtkQtChartSeriesSelectionItem> &points)
{
  if(!this->Series.isEmpty() || points.isEmpty() || this->Points.isEmpty())
    {
    return false;
    }

  bool changed = false;
  QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter = points.begin();
  for( ; iter != points.end(); ++iter)
    {
    if(iter->Series < 0 || iter->Points.isEmpty())
      {
      continue;
      }

    QList<vtkQtChartSeriesSelectionItem>::Iterator jter = this->Points.begin();
    while(jter != this->Points.end())
      {
      if(iter->Series < jter->Series)
        {
        break;
        }
      else if(iter->Series == jter->Series)
        {
        if(this->subtractRanges(iter->Points, jter->Points))
          {
          changed = true;
          if(jter->Points.isEmpty())
            {
            jter = this->Points.erase(jter);
            continue;
            }
          }
        }

      ++jter;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractPoints(
    const vtkQtChartIndexRange &series)
{
  if(!this->Series.isEmpty() || this->Points.isEmpty())
    {
    return false;
    }

  // Remove the series range along with any series points.
  bool changed = false;
  QList<vtkQtChartSeriesSelectionItem>::Iterator iter = this->Points.begin();
  while(iter != this->Points.end())
    {
    if(iter->Series >= series.first || iter->Series <= series.second)
      {
      iter = this->Points.erase(iter);
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
    const QList<vtkQtChartSeriesSelectionItem> &points)
{
  if(this->Series.isEmpty() && !points.isEmpty())
    {
    if(this->Points.isEmpty())
      {
      return this->addPoints(points);
      }
    else
      {
      bool changed = false;
      QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter;
      for(iter = points.begin(); iter != points.end(); ++iter)
        {
        if(iter->Series < 0 || iter->Points.isEmpty())
          {
          continue;
          }

        bool doAdd = true;
        QList<vtkQtChartSeriesSelectionItem>::Iterator jter =
            this->Points.begin();
        while(jter != this->Points.end())
          {
          if(iter->Series < jter->Series)
            {
            jter = this->Points.insert(jter,
                vtkQtChartSeriesSelectionItem(iter->Series));
            this->addRanges(iter->Points, jter->Points);
            changed = true;
            doAdd = false;
            break;
            }
          else if(iter->Series == jter->Series)
            {
            vtkQtChartIndexRangeList temp = iter->Points;
            this->subtractRanges(jter->Points, temp);
            this->subtractRanges(iter->Points, jter->Points);
            this->addRanges(temp, jter->Points);
            changed = true;
            doAdd = false;
            if(jter->Points.isEmpty())
              {
              this->Points.erase(jter);
              }

            break;
            }

          ++jter;
          }

        if(doAdd)
          {
          this->Points.append(vtkQtChartSeriesSelectionItem(iter->Series));
          this->addRanges(iter->Points, this->Points.last().Points);
          changed = true;
          }
        }

      return changed;
      }
    }

  return false;
}

QList<int> vtkQtChartSeriesSelection::getPointSeries() const
{
  QList<int> series;
  QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter;
  for(iter = this->Points.begin(); iter != this->Points.end(); ++iter)
    {
    series.append(iter->Series);
    }

  return series;
}

void vtkQtChartSeriesSelection::limitPoints(int series, int minimum,
    int maximum)
{
  QList<vtkQtChartSeriesSelectionItem>::Iterator iter = this->Points.begin();
  for( ; iter != this->Points.end(); ++iter)
    {
    if(iter->Series == series)
      {
      this->limitRanges(iter->Points, minimum, maximum);
      if(iter->Points.isEmpty())
        {
        this->Points.erase(iter);
        }

      break;
      }
    }
}

vtkQtChartSeriesSelection &vtkQtChartSeriesSelection::operator=(
    const vtkQtChartSeriesSelection &other)
{
  this->Series = other.Series;
  this->Points = other.Points;
  return *this;
}

bool vtkQtChartSeriesSelection::addRanges(
    const vtkQtChartIndexRangeList &source, vtkQtChartIndexRangeList &target)
{
  if(source.isEmpty())
    {
    return false;
    }

  bool changed = false;
  vtkQtChartIndexRangeList::ConstIterator iter = source.begin();
  for( ; iter != source.end(); ++iter)
    {
    // Make sure the range is in the right order.
    vtkQtChartIndexRange range = *iter;
    if(range.second < range.first)
      {
      int temp = range.second;
      range.second = range.first;
      range.first = temp;
      }

    // Add or merge the range.
    bool doAdd = true;
    vtkQtChartIndexRangeList::Iterator jter = target.begin();
    while(jter != target.end())
      {
      if(range.first <= jter->second + 1)
        {
        if(range.second < jter->first - 1)
          {
          target.insert(jter, range);
          doAdd = false;
          changed = true;
          break;
          }
        else if(range.second > jter->second)
          {
          if(jter->first < range.first)
            {
            range.first = jter->first;
            }

          jter = target.erase(jter);
          continue;
          }
        else if(range.first < jter->first)
          {
          jter->first = range.first;
          doAdd = false;
          changed = true;
          break;
          }
        }

      ++jter;
      }

    if(doAdd)
      {
      target.append(range);
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartSeriesSelection::subtractRanges(
    const vtkQtChartIndexRangeList &source, vtkQtChartIndexRangeList &target)
{
  if(target.isEmpty())
    {
    return false;
    }

  bool changed = false;
  vtkQtChartIndexRangeList::ConstIterator iter = source.begin();
  for( ; iter != source.end(); ++iter)
    {
    // Make sure the range is in the right order.
    vtkQtChartIndexRange range = *iter;
    if(range.second < range.first)
      {
      int temp = range.second;
      range.second = range.first;
      range.first = temp;
      }

    // Subtract the range if it is in the target list.
    vtkQtChartIndexRangeList::Iterator jter = target.begin();
    while(jter != target.end())
      {
      if(jter->first > range.second)
        {
        break;
        }
      else if(jter->second >= range.first)
        {
        if(range.first <= jter->first)
          {
          if(range.second >= jter->second)
            {
            jter = target.erase(jter);
            changed = true;
            continue;
            }
          else
            {
            jter->first = range.second + 1;
            changed = true;
            break;
            }
          }
        else if(range.second >= jter->second)
          {
          jter->second = range.first - 1;
          changed = true;
          }
        else
          {
          jter = target.insert(jter,
              vtkQtChartIndexRange(jter->first, range.first - 1));
          ++jter;
          jter->first = range.second + 1;
          changed = true;
          break;
          }
        }

      ++jter;
      }
    }

  return changed;
}

void vtkQtChartSeriesSelection::limitRanges(vtkQtChartIndexRangeList &list,
    int minimum, int maximum)
{
  vtkQtChartIndexRangeList::Iterator iter = list.begin();
  while(iter != list.end())
    {
    if((iter->first < minimum && iter->second < minimum) ||
        (iter->first > maximum && iter->second > maximum))
      {
      iter = list.erase(iter);
      continue;
      }

    if(iter->first < minimum)
      {
      iter->first = minimum;
      }
    else if(iter->first > maximum)
      {
      iter->first = maximum;
      }

    if(iter->second < minimum)
      {
      iter->second = minimum;
      }
    else if(iter->second > maximum)
      {
      iter->second = maximum;
      }

    ++iter;
    }
}


