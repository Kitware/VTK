/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisCornerDomain.h

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

/// \file vtkQtChartAxisCornerDomain.h
/// \date March 3, 2008

#ifndef _vtkQtChartAxisCornerDomain_h
#define _vtkQtChartAxisCornerDomain_h

#include "vtkQtChartExport.h"
#include "vtkQtChartAxis.h" // needed for enum

class vtkQtChartAxisCornerDomainInternal;
class vtkQtChartAxisDomainPriority;
class vtkQtChartSeriesDomain;


class VTKQTCHART_EXPORT vtkQtChartAxisCornerDomain
{
public:
  vtkQtChartAxisCornerDomain();
  vtkQtChartAxisCornerDomain(const vtkQtChartAxisCornerDomain &other);
  ~vtkQtChartAxisCornerDomain();

  int getNumberOfDomains() const;
  const vtkQtChartSeriesDomain *getDomain(int index) const;
  vtkQtChartSeriesDomain *getDomain(int index);

  const vtkQtChartSeriesDomain *getDomain(
      const vtkQtChartAxisDomainPriority &xPriority,
      const vtkQtChartAxisDomainPriority &yPriority) const;

  const vtkQtChartSeriesDomain *getDomain(
      vtkQtChartAxis::AxisDomain xDomain,
      const vtkQtChartAxisDomainPriority &yPriority) const;

  const vtkQtChartSeriesDomain *getDomain(
      const vtkQtChartAxisDomainPriority &xPriority,
      vtkQtChartAxis::AxisDomain yDomain) const;

  const vtkQtChartSeriesDomain *getDomain(
      vtkQtChartAxis::AxisDomain xDomain,
      vtkQtChartAxis::AxisDomain yDomain, int *index=0) const;

  bool mergeDomain(const vtkQtChartSeriesDomain &domain, int *index=0);

  void removeDomain(int index);

  void clear();

  void setHorizontalPreferences(bool padRange, bool expandToZero,
      bool addSpace);
  void setVerticalPreferences(bool padRange, bool expandToZero,
      bool addSpace);

  vtkQtChartAxisCornerDomain &operator=(
      const vtkQtChartAxisCornerDomain &other);

private:
  vtkQtChartAxisCornerDomainInternal *Internal;
};

#endif
