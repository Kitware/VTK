/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLayerDomain.cxx

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

/// \file vtkQtChartLayerDomain.cxx
/// \date March 4, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartLayerDomain.h"

#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartSeriesDomain.h"


vtkQtChartLayerDomain::vtkQtChartLayerDomain()
{
  for(int i = 0; i < 4; i++)
    {
    this->Domains[i] = 0;
    }
}

vtkQtChartLayerDomain::~vtkQtChartLayerDomain()
{
  this->clear();
}

const vtkQtChartAxisCornerDomain *vtkQtChartLayerDomain::getDomain(
    vtkQtChartLayer::AxesCorner corner) const
{
  return this->Domains[corner];
}

void vtkQtChartLayerDomain::mergeDomain(
    const vtkQtChartAxisCornerDomain &domain,
    vtkQtChartLayer::AxesCorner corner)
{
  if(this->Domains[corner] == 0)
    {
    this->Domains[corner] = new vtkQtChartAxisCornerDomain(domain);
    }
  else
    {
    for(int i = 0; i < domain.getNumberOfDomains(); i++)
      {
      this->Domains[corner]->mergeDomain(*domain.getDomain(i));
      }
    }
}

void vtkQtChartLayerDomain::clear()
{
  for(int i = 0; i < 4; i++)
    {
    if(this->Domains[i])
      {
      delete this->Domains[i];
      this->Domains[i] = 0;
      }
    }
}


