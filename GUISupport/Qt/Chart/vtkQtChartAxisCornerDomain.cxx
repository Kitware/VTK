/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisCornerDomain.cxx

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

/// \file vtkQtChartAxisCornerDomain.cxx
/// \date March 3, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisCornerDomain.h"

#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisDomainPriority.h"
#include "vtkQtChartSeriesDomain.h"
#include <QList>


class vtkQtChartAxisCornerDomainInternal
{
public:
  vtkQtChartAxisCornerDomainInternal();
  vtkQtChartAxisCornerDomainInternal(
      const vtkQtChartAxisCornerDomainInternal &other);
  ~vtkQtChartAxisCornerDomainInternal() {}

  vtkQtChartAxisCornerDomainInternal &operator=(
      const vtkQtChartAxisCornerDomainInternal &other);

  QList<vtkQtChartSeriesDomain> Domains;
  bool XPadRange;
  bool XExpandToZero;
  bool XAddSpace;
  bool YPadRange;
  bool YExpandToZero;
  bool YAddSpace;
};


//-----------------------------------------------------------------------------
vtkQtChartAxisCornerDomainInternal::vtkQtChartAxisCornerDomainInternal()
  : Domains()
{
  this->XPadRange = false;
  this->XExpandToZero = false;
  this->XAddSpace = false;
  this->YPadRange = false;
  this->YExpandToZero = false;
  this->YAddSpace = false;
}

vtkQtChartAxisCornerDomainInternal::vtkQtChartAxisCornerDomainInternal(
    const vtkQtChartAxisCornerDomainInternal &other)
  : Domains(other.Domains)
{
  this->XPadRange = other.XPadRange;
  this->XExpandToZero = other.XExpandToZero;
  this->XAddSpace = other.XAddSpace;
  this->YPadRange = other.YPadRange;
  this->YExpandToZero = other.YExpandToZero;
  this->YAddSpace = other.YAddSpace;
}

vtkQtChartAxisCornerDomainInternal &
    vtkQtChartAxisCornerDomainInternal::operator=(
    const vtkQtChartAxisCornerDomainInternal &other)
{
  this->Domains = other.Domains;
  this->XPadRange = other.XPadRange;
  this->XExpandToZero = other.XExpandToZero;
  this->XAddSpace = other.XAddSpace;
  this->YPadRange = other.YPadRange;
  this->YExpandToZero = other.YExpandToZero;
  this->YAddSpace = other.YAddSpace;
  return *this;
}


//-----------------------------------------------------------------------------
vtkQtChartAxisCornerDomain::vtkQtChartAxisCornerDomain()
{
  this->Internal = new vtkQtChartAxisCornerDomainInternal();
}

vtkQtChartAxisCornerDomain::vtkQtChartAxisCornerDomain(
    const vtkQtChartAxisCornerDomain &other)
{
  this->Internal = new vtkQtChartAxisCornerDomainInternal(*other.Internal);
}

vtkQtChartAxisCornerDomain::~vtkQtChartAxisCornerDomain()
{
  delete this->Internal;
}

int vtkQtChartAxisCornerDomain::getNumberOfDomains() const
{
  return this->Internal->Domains.size();
}

const vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(
    int index) const
{
  if(index >= 0 && index < this->Internal->Domains.size())
    {
    return &this->Internal->Domains[index];
    }

  return 0;
}

vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(int index)
{
  if(index >= 0 && index < this->Internal->Domains.size())
    {
    return &this->Internal->Domains[index];
    }

  return 0;
}

const vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(
    const vtkQtChartAxisDomainPriority &xPriority,
    const vtkQtChartAxisDomainPriority &yPriority) const
{
  int series = -1;
  int xBest = -1;
  int yBest = -1;
  int xIndex = -1;
  int yIndex = -1;
  QList<vtkQtChartSeriesDomain>::Iterator iter =
      this->Internal->Domains.begin();
  for(int i = 0; iter != this->Internal->Domains.end(); ++iter, ++i)
    {
    xIndex = xPriority.getOrder().indexOf(iter->getXDomain().getDomainType());
    yIndex = yPriority.getOrder().indexOf(iter->getYDomain().getDomainType());
    if(xBest == -1 || xIndex < xBest)
      {
      xBest = xIndex;
      yBest = yIndex;
      series = i;
      }
    else if(xIndex == xBest && yIndex < yBest)
      {
      yBest = yIndex;
      series = i;
      }
    }

  return this->getDomain(series);
}

const vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(
    vtkQtChartAxis::AxisDomain xDomain,
    const vtkQtChartAxisDomainPriority &yPriority) const
{
  int series = -1;
  int yBest = -1;
  QList<vtkQtChartSeriesDomain>::Iterator iter =
      this->Internal->Domains.begin();
  for(int i = 0; iter != this->Internal->Domains.end(); ++iter, ++i)
    {
    if(iter->getXDomain().getDomainType() == xDomain)
      {
      int yIndex = yPriority.getOrder().indexOf(
          iter->getYDomain().getDomainType());
      if(yBest == -1 || yIndex < yBest)
        {
        yBest = yIndex;
        series = i;
        }
      }
    }

  return this->getDomain(series);
}

const vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(
    const vtkQtChartAxisDomainPriority &xPriority,
    vtkQtChartAxis::AxisDomain yDomain) const
{
  int series = -1;
  int xBest = -1;
  QList<vtkQtChartSeriesDomain>::Iterator iter =
      this->Internal->Domains.begin();
  for(int i = 0; iter != this->Internal->Domains.end(); ++iter, ++i)
    {
    if(iter->getYDomain().getDomainType() == yDomain)
      {
      int xIndex = xPriority.getOrder().indexOf(
          iter->getXDomain().getDomainType());
      if(xBest == -1 || xIndex < xBest)
        {
        xBest = xIndex;
        series = i;
        }
      }
    }

  return this->getDomain(series);
}

const vtkQtChartSeriesDomain *vtkQtChartAxisCornerDomain::getDomain(
    vtkQtChartAxis::AxisDomain xDomain,
    vtkQtChartAxis::AxisDomain yDomain, int *index) const
{
  QList<vtkQtChartSeriesDomain>::Iterator iter =
      this->Internal->Domains.begin();
  for(int i = 0; iter != this->Internal->Domains.end(); ++iter, ++i)
    {
    if(iter->getXDomain().getDomainType() == xDomain &&
        iter->getYDomain().getDomainType() == yDomain)
      {
      if(index)
        {
        *index = i;
        }

      return &(*iter);
      }
    }

  return 0;
}

bool vtkQtChartAxisCornerDomain::mergeDomain(
    const vtkQtChartSeriesDomain &domain, int *index)
{
  // See if there is a compatible series domain.
  int i = 0;
  bool changed = false;
  vtkQtChartAxis::AxisDomain xType = domain.getXDomain().getDomainType();
  vtkQtChartAxis::AxisDomain yType = domain.getYDomain().getDomainType();
  QList<vtkQtChartSeriesDomain>::Iterator iter =
      this->Internal->Domains.begin();
  for( ; iter != this->Internal->Domains.end(); ++iter, ++i)
    {
    if(iter->getXDomain().getDomainType() == xType &&
        iter->getYDomain().getDomainType() == yType)
      {
      break;
      }
    }

  if(iter == this->Internal->Domains.end())
    {
    // Add a new series domain object for the new domain.
    this->Internal->Domains.append(domain);
    changed = true;

    // Set up the default preferences.
    this->Internal->Domains.last().getXDomain().setPreferences(
        this->Internal->XPadRange, this->Internal->XExpandToZero,
        this->Internal->XAddSpace);
    this->Internal->Domains.last().getYDomain().setPreferences(
        this->Internal->YPadRange, this->Internal->YExpandToZero,
        this->Internal->YAddSpace);
    }
  else
    {
    bool xChanged = iter->getXDomain().mergeDomain(domain.getXDomain());
    bool yChanged = iter->getYDomain().mergeDomain(domain.getYDomain());
    changed = xChanged || yChanged;
    }

  if(index)
    {
    *index = i;
    }

  return changed;
}

void vtkQtChartAxisCornerDomain::removeDomain(int index)
{
  if(index >= 0 && index < this->Internal->Domains.size())
    {
    this->Internal->Domains.removeAt(index);
    }
}

void vtkQtChartAxisCornerDomain::clear()
{
  this->Internal->Domains.clear();
}

void vtkQtChartAxisCornerDomain::setHorizontalPreferences(bool padRange,
    bool expandToZero, bool addSpace)
{
  this->Internal->XPadRange = padRange;
  this->Internal->XExpandToZero = expandToZero;
  this->Internal->XAddSpace = addSpace;
}

void vtkQtChartAxisCornerDomain::setVerticalPreferences(bool padRange,
    bool expandToZero, bool addSpace)
{
  this->Internal->YPadRange = padRange;
  this->Internal->YExpandToZero = expandToZero;
  this->Internal->YAddSpace = addSpace;
}

vtkQtChartAxisCornerDomain &vtkQtChartAxisCornerDomain::operator=(
    const vtkQtChartAxisCornerDomain &other)
{
  *this->Internal = *other.Internal;
  return *this;
}


