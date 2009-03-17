/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleRegistry.cxx

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

/// \file vtkQtChartStyleRegistry.cxx
/// \date March 13, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartStyleRegistry.h"

#include <QList>


class vtkQtChartStyleRegistryInternal
{
public:
  vtkQtChartStyleRegistryInternal();
  ~vtkQtChartStyleRegistryInternal() {}

  QList<int> Ids;
};


//-----------------------------------------------------------------------------
vtkQtChartStyleRegistryInternal::vtkQtChartStyleRegistryInternal()
  : Ids()
{
}


//-----------------------------------------------------------------------------
vtkQtChartStyleRegistry::vtkQtChartStyleRegistry()
{
  this->Internal = new vtkQtChartStyleRegistryInternal();
}

vtkQtChartStyleRegistry::~vtkQtChartStyleRegistry()
{
  delete this->Internal;
}

int vtkQtChartStyleRegistry::reserveStyle()
{
  int idx = this->Internal->Ids.indexOf(0);
  if(idx != -1)
    {
    this->Internal->Ids[idx] = 1;
    return idx;
    }
  
  this->Internal->Ids.append(1);
  return this->Internal->Ids.count() - 1;
}

void vtkQtChartStyleRegistry::releaseStyle(int id)
{
  if(id >= 0 && this->Internal->Ids.count() > id)
    {
    this->Internal->Ids[id] = 0;
    }

  // clean up at end
  while(this->Internal->Ids.count() &&
      this->Internal->Ids[this->Internal->Ids.count()-1] == 0)
    {
    this->Internal->Ids.removeLast();
    }
}


