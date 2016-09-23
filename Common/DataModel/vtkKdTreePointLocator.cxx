/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTreePointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKdTreePointLocator.h"

#include "vtkKdTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkKdTreePointLocator);

vtkKdTreePointLocator::vtkKdTreePointLocator()
{
  this->KdTree = 0;
}

vtkKdTreePointLocator::~vtkKdTreePointLocator()
{
  if(this->KdTree)
  {
    this->KdTree->Delete();
  }
}

vtkIdType vtkKdTreePointLocator::FindClosestPoint(const double x[3])
{
  this->BuildLocator();
  double dist2;

  return this->KdTree->FindClosestPoint(x[0], x[1], x[2], dist2);
}

vtkIdType vtkKdTreePointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  this->BuildLocator();
  return this->KdTree->FindClosestPointWithinRadius(radius, x, dist2);
}

void vtkKdTreePointLocator::FindClosestNPoints(int N, const double x[3],
                                               vtkIdList* result)
{
  this->BuildLocator();
  this->KdTree->FindClosestNPoints(N, x, result);
}

void vtkKdTreePointLocator::FindPointsWithinRadius(double R, const double x[3],
                                                   vtkIdList * result)
{
  this->BuildLocator();
  this->KdTree->FindPointsWithinRadius(R, x, result);
}

void vtkKdTreePointLocator::FreeSearchStructure()
{
  if(this->KdTree)
  {
    this->KdTree->Delete();
    this->KdTree = 0;
  }
}

void vtkKdTreePointLocator::BuildLocator()
{
  if(!this->KdTree)
  {
    vtkPointSet* pointSet = vtkPointSet::SafeDownCast(this->GetDataSet());
    if(!pointSet)
    {
      vtkErrorMacro("vtkKdTreePointLocator requires a PointSet to build locator.");
      return;
    }
    this->KdTree = vtkKdTree::New();
    this->KdTree->BuildLocatorFromPoints(pointSet);
    this->KdTree->GetBounds(this->Bounds);
    this->Modified();
  }
}

void vtkKdTreePointLocator::GenerateRepresentation(int level, vtkPolyData *pd)
{
  this->BuildLocator();
  this->KdTree->GenerateRepresentation(level, pd);
}

void vtkKdTreePointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "KdTree " << this->KdTree << "\n";
}

