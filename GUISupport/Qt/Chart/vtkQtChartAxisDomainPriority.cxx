/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisDomainPriority.cxx

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

/// \file vtkQtChartAxisDomainPriority.cxx
/// \date February 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisDomainPriority.h"


vtkQtChartAxisDomainPriority::vtkQtChartAxisDomainPriority()
  : Order()
{
  this->Order = this->getDefaultOrder();
}

vtkQtChartAxisDomainPriority::vtkQtChartAxisDomainPriority(
    const vtkQtChartAxisDomainPriority &other)
  : Order(other.Order)
{
}

QList<int> vtkQtChartAxisDomainPriority::getDefaultOrder() const
{
  QList<int> order;
  order.append(vtkQtChartAxisDomainPriority::Number);
  order.append(vtkQtChartAxisDomainPriority::String);
  order.append(vtkQtChartAxisDomainPriority::Date);
  order.append(vtkQtChartAxisDomainPriority::Time);
  return order;
}

void vtkQtChartAxisDomainPriority::setOrder(const QList<int> &order)
{
  // Make sure the priority list includes all the types. Fill in the
  // blanks with the default order.
  QList<int> defaultOrder = this->getDefaultOrder();

  int i = 0;
  QList<int>::ConstIterator iter = order.begin();
  for( ; iter != order.end() && i < this->Order.size(); ++iter)
    {
    if(defaultOrder.contains(*iter))
      {
      defaultOrder.removeAll(*iter);
      this->Order[i++] = *iter;
      }
    }

  for(iter = defaultOrder.begin(); iter != defaultOrder.end(); ++iter)
    {
    this->Order[i++] = *iter;
    }
}

vtkQtChartAxisDomainPriority &vtkQtChartAxisDomainPriority::operator=(
    const vtkQtChartAxisDomainPriority &other)
{
  this->Order = other.Order;
  return *this;
}

bool vtkQtChartAxisDomainPriority::operator==(
    const vtkQtChartAxisDomainPriority &other) const
{
  return this->Order == other.Order;
}

bool vtkQtChartAxisDomainPriority::operator!=(
    const vtkQtChartAxisDomainPriority &other) const
{
  return this->Order != other.Order;
}


