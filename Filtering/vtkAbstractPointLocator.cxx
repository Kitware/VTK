/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractPointLocator.h"

#include "vtkDataSet.h"
#include "vtkIdList.h"


vtkAbstractPointLocator::vtkAbstractPointLocator()
{
  for(int i=0;i<6;i++)
    {
    this->Bounds[i] = 0;
    }
}

vtkAbstractPointLocator::~vtkAbstractPointLocator()
{
}


// Given a position x-y-z, return the id of the point closest to it.
vtkIdType vtkAbstractPointLocator::FindClosestPoint(double x, double y, double z)
{
  double xyz[3];

  xyz[0] = x; xyz[1] = y; xyz[2] = z;
  return this->FindClosestPoint(xyz);
}

void vtkAbstractPointLocator::FindClosestNPoints(int N, double x,
                                         double y, double z,
                                         vtkIdList *result)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->FindClosestNPoints(N,p,result);
}

void vtkAbstractPointLocator::FindPointsWithinRadius(double R, double x,
                                             double y, double z,
                                             vtkIdList *result)
{
  double p[3];
  p[0] = x;
  p[1] = y;
  p[2] = z;
  this->FindPointsWithinRadius(R,p,result);
}

void vtkAbstractPointLocator::GetBounds(double* bnds)
{
  for(int i=0;i<6;i++)
    {
    bnds[i] = this->Bounds[i];
    }
}

void vtkAbstractPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  for(int i=0;i<6;i++)
    {
    os << indent << "Bounds[" << i << "]: " << this->Bounds[i] << "\n";
    }
}

