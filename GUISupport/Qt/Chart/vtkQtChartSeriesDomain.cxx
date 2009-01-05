/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesDomain.cxx

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

/// \file vtkQtChartSeriesDomain.cxx
/// \date March 3, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesDomain.h"

#include "vtkQtChartAxisDomain.h"


class vtkQtChartSeriesDomainInternal
{
public:
  vtkQtChartSeriesDomainInternal();
  vtkQtChartSeriesDomainInternal(const vtkQtChartSeriesDomainInternal &other);
  ~vtkQtChartSeriesDomainInternal() {}

  vtkQtChartAxisDomain XDomain;
  vtkQtChartAxisDomain YDomain;
};


//-----------------------------------------------------------------------------
vtkQtChartSeriesDomainInternal::vtkQtChartSeriesDomainInternal()
  : XDomain(), YDomain()
{
}

vtkQtChartSeriesDomainInternal::vtkQtChartSeriesDomainInternal(
    const vtkQtChartSeriesDomainInternal &other)
  : XDomain(other.XDomain), YDomain(other.YDomain)
{
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesDomain::vtkQtChartSeriesDomain()
{
  this->Internal = new vtkQtChartSeriesDomainInternal();
}

vtkQtChartSeriesDomain::vtkQtChartSeriesDomain(
    const vtkQtChartSeriesDomain &other)
{
  this->Internal = new vtkQtChartSeriesDomainInternal(*other.Internal);
}

vtkQtChartSeriesDomain::~vtkQtChartSeriesDomain()
{
  delete this->Internal;
}

const vtkQtChartAxisDomain &vtkQtChartSeriesDomain::getXDomain() const
{
  return this->Internal->XDomain;
}

vtkQtChartAxisDomain &vtkQtChartSeriesDomain::getXDomain()
{
  return this->Internal->XDomain;
}

const vtkQtChartAxisDomain &vtkQtChartSeriesDomain::getYDomain() const
{
  return this->Internal->YDomain;
}

vtkQtChartAxisDomain &vtkQtChartSeriesDomain::getYDomain()
{
  return this->Internal->YDomain;
}

vtkQtChartSeriesDomain &vtkQtChartSeriesDomain::operator=(
    const vtkQtChartSeriesDomain &other)
{
  this->Internal->XDomain = other.Internal->XDomain;
  this->Internal->YDomain = other.Internal->YDomain;
  return *this;
}


