/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleManager.cxx

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

/// \file vtkQtChartStyleManager.cxx
/// \date February 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartStyleManager.h"

#include "vtkQtChartStyleGenerator.h"
#include "vtkQtChartColorStyleGenerator.h"
#include <QList>


//----------------------------------------------------------------------------
vtkQtChartStyleManager::vtkQtChartStyleManager(QObject *parentObject)
  : QObject(parentObject), Ids()
{
  this->DefaultGenerator = new vtkQtChartColorStyleGenerator(this,
      vtkQtChartColors::Spectrum);
  this->Generator = this->DefaultGenerator;
}

vtkQtChartStyleGenerator *vtkQtChartStyleManager::getGenerator()
{
  return this->Generator;
}

void vtkQtChartStyleManager::setGenerator(
    vtkQtChartStyleGenerator *generator)
{
  this->Generator = generator;
  if(this->Generator == 0)
    {
    this->Generator = this->DefaultGenerator;
    }
}

int vtkQtChartStyleManager::reserveStyle()
{
  int idx = this->Ids.indexOf(0);
  if(idx != -1)
    {
    this->Ids[idx] = 1;
    return idx;
    }
  
  this->Ids.append(1);
  return this->Ids.count() - 1;
}

void vtkQtChartStyleManager::releaseStyle(int id)
{
  if(id >= 0 && this->Ids.count() > id)
    {
    this->Ids[id] = 0;
    }

  // clean up at end
  while(this->Ids.count() && this->Ids[this->Ids.count()-1] == 0)
    {
    this->Ids.removeLast();
    }
}


