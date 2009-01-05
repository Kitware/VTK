/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisDomain.cxx

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

/// \file vtkQtChartAxisDomain.cxx
/// \date February 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisDomain.h"

#include <QDate>
#include <QDateTime>
#include <QLinkedList>
#include <QPair>
#include <QTime>
#include <QString>


vtkQtChartAxisDomain::vtkQtChartAxisDomain()
  : List(), Range()
{
  this->PadRange = false;
  this->ExpandToZero = false;
  this->AddSpace = false;
}

vtkQtChartAxisDomain::vtkQtChartAxisDomain(const vtkQtChartAxisDomain &other)
  : List(other.List), Range(other.Range)
{
  this->PadRange = other.PadRange;
  this->ExpandToZero = other.ExpandToZero;
  this->AddSpace = other.AddSpace;
}

bool vtkQtChartAxisDomain::isEmpty() const
{
  return this->List.isEmpty() && this->Range.isEmpty();
}

bool vtkQtChartAxisDomain::isRangeInList() const
{
  bool inList = false;
  if(!this->Range.isEmpty() && !this->List.isEmpty())
    {
    if(this->List[0].type() == QVariant::Double ||
        this->Range[0].type() == QVariant::Double)
      {
      inList = this->Range[0].toDouble() >= this->List[0].toDouble() &&
          this->Range[1].toDouble() <= this->List.last().toDouble();
      }
    else if(this->List[0].type() == QVariant::Int)
      {
      inList = this->Range[0].toInt() >= this->List[0].toInt() &&
          this->Range[1].toInt() <= this->List.last().toInt();
      }
    else if(this->List[0].type() == QVariant::DateTime ||
        this->Range[0].type() == QVariant::DateTime)
      {
      inList = this->Range[0].toDateTime() >= this->List[0].toDateTime() &&
          this->Range[1].toDateTime() <= this->List.last().toDateTime();
      }
    else if(this->List[0].type() == QVariant::Date)
      {
      inList = this->Range[0].toDate() >= this->List[0].toDate() &&
          this->Range[1].toDate() <= this->List.last().toDate();
      }
    else if(this->List[0].type() == QVariant::Time)
      {
      inList = this->Range[0].toTime() >= this->List[0].toTime() &&
          this->Range[1].toTime() <= this->List.last().toTime();
      }
    }

  return inList;
}

vtkQtChartAxis::AxisDomain vtkQtChartAxisDomain::getDomainType() const
{
  return vtkQtChartAxisDomain::getAxisDomain(this->getVariantType());
}

QVariant::Type vtkQtChartAxisDomain::getVariantType() const
{
  QVariant::Type domain = QVariant::Invalid;
  if(this->List.size() > 0)
    {
    domain = this->List[0].type();
    }

  if(this->Range.size() > 0 && (domain == QVariant::Invalid ||
      domain == QVariant::Int || domain == QVariant::Date))
    {
    // Double supercedes int and DateTime supercedes Date.
    domain = this->Range[0].type();
    }

  return domain;
}

bool vtkQtChartAxisDomain::isTypeCompatible(QVariant::Type domain) const
{
  QVariant::Type current = this->getVariantType();
  return current == domain || current == QVariant::Invalid ||
      (current == QVariant::Int && domain == QVariant::Double) ||
      (current == QVariant::Double && domain == QVariant::Int) ||
      (current == QVariant::Date && domain == QVariant::DateTime) ||
      (current == QVariant::DateTime && domain == QVariant::Date);
}

const QList<QVariant> &vtkQtChartAxisDomain::getDomain(bool &isRange) const
{
  if(!this->List.isEmpty() && !this->Range.isEmpty())
    {
    // Return the list if the range is inside the list.
    if(this->isRangeInList())
      {
      isRange = false;
      return this->List;
      }
    else
      {
      isRange = true;
      return this->Range;
      }
    }
  else if(!this->Range.isEmpty())
    {
    isRange = true;
    return this->Range;
    }
  else
    {
    isRange = false;
    return this->List;
    }
}

void vtkQtChartAxisDomain::setRange(const QList<QVariant> &range)
{
  if(range.size() == 2)
    {
    // See if the domain list needs to be cleared.
    if(this->List.size() > 0 && !this->isTypeCompatible(range[0].type()))
      {
      this->List.clear();
      }

    this->Range = range;
    }
  else
    {
    this->Range.clear();
    }
}

void vtkQtChartAxisDomain::setDomain(const QList<QVariant> &domain)
{
  if(domain.size() > 0)
    {
    // See if the domain list needs to be cleared.
    if(this->Range.size() > 0 && !this->isTypeCompatible(domain[0].type()))
      {
      this->Range.clear();
      }

    this->List = domain;
    }
  else
    {
    this->List.clear();
    }
}

bool vtkQtChartAxisDomain::mergeRange(const QList<QVariant> &range)
{
  if(range.size() == 2)
    {
    // Use the first object in the list to determine the type. Make
    // sure the types are compatible.
    QVariant::Type rangeType = range[0].type();
    if(!this->isTypeCompatible(rangeType))
      {
      return false;
      }

    // Use the appropriate method to merge the range.
    if(rangeType == QVariant::Int || rangeType == QVariant::Double)
      {
      return this->mergeNumberRange(range);
      }
    else if(rangeType == QVariant::Time)
      {
      return this->mergeTimeRange(range);
      }
    else if(rangeType == QVariant::Date || rangeType == QVariant::DateTime)
      {
      return this->mergeDateRange(range);
      }
    }
  else
    {
    return this->mergeDomain(range);
    }

  return false;
}

bool vtkQtChartAxisDomain::mergeDomain(const QList<QVariant> &domain)
{
  if(domain.size() > 0)
    {
    // Use the first object in the list to determine the type. Make
    // sure the types are compatible.
    QVariant::Type domainType = domain[0].type();
    if(!this->isTypeCompatible(domainType))
      {
      return false;
      }

    // Use the appropriate method to merge the domain.
    if(domainType == QVariant::String)
      {
      return this->mergeStringDomain(domain);
      }
    else if(domainType == QVariant::Int || domainType == QVariant::Double)
      {
      return this->mergeNumberDomain(domain);
      }
    else if(domainType == QVariant::Time)
      {
      return this->mergeTimeDomain(domain);
      }
    else if(domainType == QVariant::Date || domainType == QVariant::DateTime)
      {
      return this->mergeDateDomain(domain);
      }
    }

  return false;
}

bool vtkQtChartAxisDomain::mergeDomain(const vtkQtChartAxisDomain &other)
{
  bool rangeChanged = this->mergeRange(other.Range);
  bool listChanged = this->mergeDomain(other.List);

  // Merge the domain layout options.
  this->setPreferences(this->PadRange || other.PadRange,
      this->ExpandToZero || other.ExpandToZero,
      this->AddSpace || other.AddSpace);

  return rangeChanged || listChanged;
}

void vtkQtChartAxisDomain::clear()
{
  this->List.clear();
  this->Range.clear();
}

void vtkQtChartAxisDomain::setPreferences(bool padRange, bool expandToZero,
    bool addSpace)
{
  this->PadRange = padRange;
  this->ExpandToZero = expandToZero;
  this->AddSpace = addSpace;
}

vtkQtChartAxisDomain &vtkQtChartAxisDomain::operator=(
    const vtkQtChartAxisDomain &other)
{
  this->List = other.List;
  this->Range = other.Range;
  this->PadRange = other.PadRange;
  this->ExpandToZero = other.ExpandToZero;
  this->AddSpace = other.AddSpace;
  return *this;
}

vtkQtChartAxis::AxisDomain vtkQtChartAxisDomain::getAxisDomain(
    QVariant::Type domain)
{
  if(domain == QVariant::String)
    {
    return vtkQtChartAxis::String;
    }
  else if(domain == QVariant::Int || domain == QVariant::Double)
    {
    return vtkQtChartAxis::Number;
    }
  else if(domain == QVariant::Time)
    {
    return vtkQtChartAxis::Time;
    }
  else if(domain == QVariant::Date || domain == QVariant::DateTime)
    {
    return vtkQtChartAxis::Date;
    }

  return vtkQtChartAxis::UnsupportedDomain;
}

void vtkQtChartAxisDomain::sort(QList<QVariant> &list)
{
  if(list.size() < 2)
    {
    return;
    }

  // Only certain domains will be sorted.
  QVariant::Type domain = list[0].type();
  if(domain != QVariant::Int && domain != QVariant::Double &&
      domain != QVariant::Date && domain != QVariant::DateTime &&
      domain != QVariant::Time)
    {
    return;
    }

  // Use a boundary list to avoid recursive calls.
  QVariant temp;
  QLinkedList<QPair<int, int> > bounds;
  bounds.append(QPair<int, int>(0, list.size() - 1));
  while(bounds.size() > 0)
    {
    QMutableLinkedListIterator<QPair<int, int> > jter(bounds);
    while(jter.hasNext())
      {
      QPair<int, int> range = jter.next();
      jter.remove();
      int length = range.second - range.first + 1;
      if(length == 2)
        {
        // Swap the values if necessary.
        bool lessThan = false;
        if(domain == QVariant::Int)
          {
          lessThan = list[range.second].toInt() < list[range.first].toInt();
          }
        else if(domain == QVariant::Double)
          {
          lessThan =
              list[range.second].toDouble() < list[range.first].toDouble();
          }
        else if(domain == QVariant::Date)
          {
          lessThan = list[range.second].toDate() < list[range.first].toDate();
          }
        else if(domain == QVariant::DateTime)
          {
          lessThan =
              list[range.second].toDateTime() < list[range.first].toDateTime();
          }
        else if(domain == QVariant::Time)
          {
          lessThan = list[range.second].toTime() < list[range.first].toTime();
          }

        if(lessThan)
          {
          temp = list[range.second];
          list[range.second] = list[range.first];
          list[range.first] = temp;
          }
        }
      else
        {
        // Use the middle value as the pivot.
        int pivot = range.first + length / 2;

        // Swap the pivot and last value.
        temp = list[range.second];
        list[range.second] = list[pivot];
        list[pivot] = temp;

        // Partition the remaining values.
        pivot = range.first;
        for(int i = range.first; i < range.second; i++)
          {
          bool lessThanEqual = false;
          if(domain == QVariant::Int)
            {
            lessThanEqual = list[i].toInt() <= list[range.second].toInt();
            }
          else if(domain == QVariant::Double)
            {
            lessThanEqual =
                list[i].toDouble() <= list[range.second].toDouble();
            }
          else if(domain == QVariant::Date)
            {
            lessThanEqual = list[i].toDate() <= list[range.second].toDate();
            }
          else if(domain == QVariant::DateTime)
            {
            lessThanEqual =
                list[i].toDateTime() <= list[range.second].toDateTime();
            }
          else if(domain == QVariant::Time)
            {
            lessThanEqual = list[i].toTime() <= list[range.second].toTime();
            }

          if(lessThanEqual)
            {
            // Swap the values if necessary.
            if(pivot != i)
              {
              temp = list[i];
              list[i] = list[pivot];
              list[pivot] = temp;
              }

            pivot++;
            }
          }

        // Move the last point to the partition.
        if(pivot != range.second)
          {
          temp = list[range.second];
          list[range.second] = list[pivot];
          list[pivot] = temp;
          }

        // Add ranges to the bounds list for the two partitions.
        length = pivot - range.first;
        if(length > 1)
          {
          jter.insert(QPair<int, int>(range.first, pivot - 1));
          }

        length = range.second - pivot;
        if(length > 1)
          {
          jter.insert(QPair<int, int>(pivot + 1, range.second));
          }
        }
      }
    }

  // Remove any duplicate values from the list.
  QList<QVariant>::Iterator iter = list.begin();
  temp = *iter;
  ++iter;
  while(iter != list.end())
    {
    if(temp == *iter)
      {
      iter = list.erase(iter);
      }
    else
      {
      temp = *iter;
      ++iter;
      }
    }
}

bool vtkQtChartAxisDomain::mergeNumberRange(const QList<QVariant> &range)
{
  if(this->Range.size() == 0)
    {
    this->Range = range;
    return true;
    }

  bool changed = false;
  if(range[0].type() == QVariant::Double &&
      this->Range[0].type() == QVariant::Int)
    {
    // If a domain has doubles, it should be used instead of ints.
    this->Range[0].convert(QVariant::Double);
    this->Range[1].convert(QVariant::Double);
    changed = true;
    }

  if(this->Range[0].type() == QVariant::Int)
    {
    int number1 = this->Range[0].toInt();
    int number2 = range[0].toInt();
    if(number2 < number1)
      {
      this->Range[0] = number2;
      changed = true;
      }

    number1 = this->Range[1].toInt();
    number2 = range[1].toInt();
    if(number2 > number1)
      {
      this->Range[1] = number2;
      changed = true;
      }
    }
  else
    {
    double number1 = this->Range[0].toDouble();
    double number2 = range[0].toDouble();
    if(number2 < number1)
      {
      this->Range[0] = number2;
      changed = true;
      }

    number1 = this->Range[1].toDouble();
    number2 = range[1].toDouble();
    if(number2 > number1)
      {
      this->Range[1] = number2;
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeNumberDomain(const QList<QVariant> &domain)
{
  // If the new list is using doubles, upgrade the current list.
  bool changed = false;
  QList<QVariant>::Iterator iter;
  if(domain[0].type() == QVariant::Double && this->List.size() > 0 &&
      this->List[0].type() == QVariant::Int)
    {
    changed = true;
    for(iter = this->List.begin(); iter != this->List.end(); ++iter)
      {
      iter->convert(QVariant::Double);
      }
    }

  // The two lists should be sorted and unique.
  if(this->List.size() == 0)
    {
    this->List = domain;
    return true;
    }

  iter = this->List.begin();
  QList<QVariant>::ConstIterator jter = domain.begin();
  while(iter != this->List.end() && jter != domain.end())
    {
    if(jter->toDouble() < iter->toDouble())
      {
      iter = this->List.insert(iter, *jter);
      ++iter;
      ++jter;
      changed = true;
      }
    else if(jter->toDouble() == iter->toDouble())
      {
      ++jter;
      }
    else
      {
      ++iter;
      }
    }

  // Add the remaining domain items to the list.
  for( ; jter != domain.end(); ++jter)
    {
    this->List.append(*jter);
    changed = true;
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeStringDomain(const QList<QVariant> &domain)
{
  bool changed = false;
  QList<QVariant>::Iterator jter;
  QList<QVariant>::ConstIterator iter = domain.begin();
  for( ; iter != domain.end(); ++iter)
    {
    for(jter = this->List.begin(); jter != this->List.end(); ++jter)
      {
      if(iter->toString() == jter->toString())
        {
        break;
        }
      }

    // Only add the string if it is not in the list.
    if(jter == this->List.end())
      {
      changed = true;
      this->List.append(*iter);
      }
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeDateRange(const QList<QVariant> &range)
{
  if(this->Range.size() == 0)
    {
    this->Range = range;
    return true;
    }

  bool changed = false;
  if(range[0].type() == QVariant::DateTime &&
      this->Range[0].type() == QVariant::Date)
    {
    // If a domain uses date-time, it should be used instead of date.
    this->Range[0].convert(QVariant::DateTime);
    this->Range[1].convert(QVariant::DateTime);
    }

  if(this->Range[0].type() == QVariant::Date)
    {
    QDate date1 = this->Range[0].toDate();
    QDate date2 = range[0].toDate();
    if(date2 < date1)
      {
      this->Range[0] = date2;
      changed = true;
      }

    date1 = this->Range[1].toDate();
    date2 = range[1].toDate();
    if(date2 > date1)
      {
      this->Range[1] = date2;
      changed = true;
      }
    }
  else
    {
    QDateTime date1 = this->Range[0].toDateTime();
    QDateTime date2 = range[0].toDateTime();
    if(date2 < date1)
      {
      this->Range[0] = date2;
      changed = true;
      }

    date1 = this->Range[1].toDateTime();
    date2 = range[1].toDateTime();
    if(date2 > date1)
      {
      this->Range[1] = date2;
      changed = true;
      }
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeDateDomain(const QList<QVariant> &domain)
{
  // If the new list is using date-time, upgrade the current list.
  bool changed = false;
  QList<QVariant>::Iterator iter;
  if(domain[0].type() == QVariant::DateTime && this->List.size() > 0 &&
      this->List[0].type() == QVariant::Date)
    {
    changed = true;
    for(iter = this->List.begin(); iter != this->List.end(); ++iter)
      {
      iter->convert(QVariant::DateTime);
      }
    }

  // The two lists should be sorted and unique.
  if(this->List.size() == 0)
    {
    this->List = domain;
    return true;
    }

  iter = this->List.begin();
  QList<QVariant>::ConstIterator jter = domain.begin();
  while(iter != this->List.end() && jter != domain.end())
    {
    bool lessThan = false;
    bool equal = false;
    if(iter->type() == QVariant::DateTime)
      {
      lessThan = jter->toDateTime() < iter->toDateTime();
      equal = jter->toDateTime() == iter->toDateTime();
      }
    else
      {
      lessThan = jter->toDate() < iter->toDate();
      equal = jter->toDate() == iter->toDate();
      }

    if(lessThan)
      {
      iter = this->List.insert(iter, *jter);
      ++iter;
      ++jter;
      changed = true;
      }
    else if(equal)
      {
      ++jter;
      }
    else
      {
      ++iter;
      }
    }

  // Add the remaining domain items to the list.
  for( ; jter != domain.end(); ++jter)
    {
    this->List.append(*jter);
    changed = true;
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeTimeRange(const QList<QVariant> &range)
{
  if(this->Range.size() == 0)
    {
    this->Range = range;
    return true;
    }

  bool changed = false;
  QTime time1 = this->Range[0].toTime();
  QTime time2 = range[0].toTime();
  if(time2 < time1)
    {
    this->Range[0] = time2;
    changed = true;
    }

  time1 = this->Range[1].toTime();
  time2 = range[1].toTime();
  if(time2 > time1)
    {
    this->Range[1] = time2;
    changed = true;
    }

  return changed;
}

bool vtkQtChartAxisDomain::mergeTimeDomain(const QList<QVariant> &domain)
{
  // The two lists should be sorted and unique.
  if(this->List.size() == 0)
    {
    this->List = domain;
    return true;
    }

  bool changed = false;
  QList<QVariant>::Iterator iter = this->List.begin();
  QList<QVariant>::ConstIterator jter = domain.begin();
  while(iter != this->List.end() && jter != domain.end())
    {
    if(jter->toTime() < iter->toTime())
      {
      iter = this->List.insert(iter, *jter);
      ++iter;
      ++jter;
      changed = true;
      }
    else if(jter->toTime() == iter->toTime())
      {
      ++jter;
      }
    else
      {
      ++iter;
      }
    }

  // Add the remaining domain items to the list.
  for( ; jter != domain.end(); ++jter)
    {
    this->List.append(*jter);
    changed = true;
    }

  return changed;
}


