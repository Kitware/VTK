/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisDomainPriority.h

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

/// \file vtkQtChartAxisDomainPriority.h
/// \date February 14, 2008

#ifndef _vtkQtChartAxisDomainPriority_h
#define _vtkQtChartAxisDomainPriority_h

#include "vtkQtChartExport.h"
#include <QList> // Needed for parameter and return type.


class VTKQTCHART_EXPORT vtkQtChartAxisDomainPriority
{
public:
  enum DomainType
    {
    Number = 0,
    Date,
    Time,
    String
    };

public:
  vtkQtChartAxisDomainPriority();
  vtkQtChartAxisDomainPriority(const vtkQtChartAxisDomainPriority &other);
  ~vtkQtChartAxisDomainPriority() {}

  QList<int> getDefaultOrder() const;

  const QList<int> &getOrder() const {return this->Order;}
  void setOrder(const QList<int> &order);

  vtkQtChartAxisDomainPriority &operator=(
      const vtkQtChartAxisDomainPriority &other);
  bool operator==(const vtkQtChartAxisDomainPriority &other) const;
  bool operator!=(const vtkQtChartAxisDomainPriority &other) const;

private:
  QList<int> Order;
};

#endif
