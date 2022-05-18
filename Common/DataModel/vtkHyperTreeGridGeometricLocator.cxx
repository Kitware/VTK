/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeoemtricLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometricLocator.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGeometricLocator);

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometricLocator::vtkHyperTreeGridGeometricLocator()
  : vtkHyperTreeGridLocator()
{
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::Search(const double point[3])
{
  return -1;
} // Search

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::FindCell(const double point[3], const double tol,
  vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  return -1;
} // FindCell

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometricLocator::IntersectWithLine(const double p0[3], const double p[2],
  const double tol, double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId,
  vtkGenericCell* cell)
{
  return 0;
} // IntersectWithLine

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::IsInExtent(const double pt[3], const double extent[6])
{
  return false;
} // IsInExtent
