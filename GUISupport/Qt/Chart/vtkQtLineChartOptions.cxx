/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartOptions.cxx

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

/// \file vtkQtLineChartOptions.cxx
/// \date June 6, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtLineChartOptions.h"

#include "vtkQtChartHelpFormatter.h"


vtkQtLineChartOptions::vtkQtLineChartOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Help = new vtkQtChartHelpFormatter("%s: %1, %2");
}

vtkQtLineChartOptions::vtkQtLineChartOptions(
    const vtkQtLineChartOptions &other)
  : QObject()
{
  this->Help = new vtkQtChartHelpFormatter(other.Help->getFormat());
}

vtkQtLineChartOptions::~vtkQtLineChartOptions()
{
  delete this->Help;
}

vtkQtLineChartOptions &vtkQtLineChartOptions::operator=(
    const vtkQtLineChartOptions &other)
{
  this->Help->setFormat(other.Help->getFormat());
  return *this;
}


