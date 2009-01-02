/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesDomainGroup.cxx

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

/// \file vtkQtChartSeriesDomainGroup.cxx
/// \date March 6, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesDomainGroup.h"


vtkQtChartSeriesDomainGroup::vtkQtChartSeriesDomainGroup(bool sortSeries)
  : Groups(), ToSort()
{
  this->SortSeries = sortSeries;
}

int vtkQtChartSeriesDomainGroup::getNumberOfGroups() const
{
  return this->Groups.size();
}

int vtkQtChartSeriesDomainGroup::getNumberOfSeries(int group) const
{
  if(group >= 0 && group < this->Groups.size())
    {
    return this->Groups[group].size();
    }

  return 0;
}

QList<int> vtkQtChartSeriesDomainGroup::getGroup(int group) const
{
  if(group >= 0 && group < this->Groups.size())
    {
    return this->Groups[group];
    }

  return QList<int>();
}

int vtkQtChartSeriesDomainGroup::findGroup(int series) const
{
  QList<QList<int> >::ConstIterator iter = this->Groups.begin();
  for(int i = 0; iter != this->Groups.end(); ++iter, ++i)
    {
    QList<int>::ConstIterator jter = iter->begin();
    for( ; jter != iter->end(); ++jter)
      {
      if(series == *jter)
        {
        return i;
        }
      }
    }

  return -1;
}

void vtkQtChartSeriesDomainGroup::prepareInsert(int seriesFirst,
    int seriesLast)
{
  // Increase the series index for all series with indexes after the
  // insertion range.
  int diff = seriesLast - seriesFirst + 1;
  QList<QList<int> >::Iterator iter = this->Groups.begin();
  for( ; iter != this->Groups.end(); ++iter)
    {
    QList<int>::Iterator jter = iter->begin();
    for( ; jter != iter->end(); ++jter)
      {
      if(*jter >= seriesFirst)
        {
        *jter += diff;
        }
      }
    }
}

void vtkQtChartSeriesDomainGroup::insertSeries(int series, int group)
{
  if(group < 0)
    {
    group = 0;
    }

  if(group >= this->Groups.size())
    {
    group = this->Groups.size();
    this->insertGroup(group);
    }

  if(this->SortSeries)
    {
    this->ToSort[group].append(series);
    }
  else
    {
    this->Groups[group].append(series);
    }
}

void vtkQtChartSeriesDomainGroup::finishInsert()
{
  if(this->SortSeries)
    {
    QList<QList<int> >::Iterator iter = this->Groups.begin();
    QList<QList<int> >::Iterator jter = this->ToSort.begin();
    for( ; iter != this->Groups.end(); ++iter, ++jter)
      {
      // First, get the list to be sorted in order.
      // TODO: Use a non-recursive quick sort.
      qSort(jter->begin(), jter->end());

      // Merge the newly added series to the group list.
      vtkQtChartSeriesDomainGroup::mergeSeriesLists(*iter, *jter);
      jter->clear();
      }
    }
}

int vtkQtChartSeriesDomainGroup::removeSeries(int series)
{
  QList<QList<int> >::Iterator iter = this->Groups.begin();
  for(int i = 0; iter != this->Groups.end(); ++iter, ++i)
    {
    QList<int>::Iterator jter = iter->begin();
    for( ; jter != iter->end(); ++jter)
      {
      if(series == *jter)
        {
        iter->erase(jter);
        return i;
        }
      }
    }

  return -1;
}

void vtkQtChartSeriesDomainGroup::finishRemoval(int seriesFirst,
    int seriesLast)
{
  // Decrease the series index for all series with indexes after the
  // removed range. Remove any empty groups.
  bool doUpdate = seriesFirst != -1 && seriesLast != -1;
  int diff = seriesLast - seriesFirst + 1;
  int i = 0;
  QList<int>::Iterator jter;
  QList<QList<int> >::Iterator iter = this->Groups.begin();
  while(iter != this->Groups.end())
    {
    if(iter->size() == 0)
      {
      iter = this->Groups.erase(iter);
      this->removeGroup(i);
      }
    else if(doUpdate)
      {
      for(jter = iter->begin(); jter != iter->end(); ++jter)
        {
        if(*jter > seriesLast)
          {
          *jter -= diff;
          }
        }

      ++iter;
      ++i;
      }
    else
      {
      ++iter;
      ++i;
      }
    }
}

void vtkQtChartSeriesDomainGroup::clear()
{
  this->Groups.clear();
  this->ToSort.clear();
}

void vtkQtChartSeriesDomainGroup::mergeSeriesLists(QList<int> &target,
    const QList<int> &source)
{
  if(target.size() == 0)
    {
    target = source;
    return;
    }

  QList<int>::Iterator iter = target.begin();
  QList<int>::ConstIterator jter = source.begin();
  while(iter != target.end() && jter != source.end())
    {
    if(*jter < *iter)
      {
      iter = target.insert(iter, *jter);
      ++jter;
      }

    ++iter;
    }

  // Add the remaining source items to the target list.
  for( ; jter != source.end(); ++jter)
    {
    target.append(*jter);
    }
}

void vtkQtChartSeriesDomainGroup::insertGroup(int group)
{
  this->Groups.insert(group, QList<int>());
  if(this->SortSeries)
    {
    this->ToSort.insert(group, QList<int>());
    }
}

void vtkQtChartSeriesDomainGroup::removeGroup(int group)
{
  if(this->SortSeries)
    {
    this->ToSort.removeAt(group);
    }
}


