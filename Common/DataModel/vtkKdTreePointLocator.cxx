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

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkKdTreePointLocator);

//------------------------------------------------------------------------------
vtkKdTreePointLocator::vtkKdTreePointLocator()
{
  this->KdTree = nullptr;
}

//------------------------------------------------------------------------------
vtkKdTreePointLocator::~vtkKdTreePointLocator()
{
  if (this->KdTree)
  {
    this->KdTree->Delete();
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkKdTreePointLocator::FindClosestPoint(const double x[3])
{
  this->BuildLocator();
  double dist2;

  return this->KdTree->FindClosestPoint(x[0], x[1], x[2], dist2);
}

//------------------------------------------------------------------------------
vtkIdType vtkKdTreePointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  this->BuildLocator();
  return this->KdTree->FindClosestPointWithinRadius(radius, x, dist2);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  this->BuildLocator();
  this->KdTree->FindClosestNPoints(N, x, result);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  this->BuildLocator();
  this->KdTree->FindPointsWithinRadius(R, x, result);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FreeSearchStructure()
{
  if (this->KdTree)
  {
    this->KdTree->Delete();
    this->KdTree = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->KdTree && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->KdTree && this->UseExistingSearchStructure)
  {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::BuildLocatorInternal()
{
  if (!this->DataSet || (this->DataSet->GetNumberOfPoints()) < 1)
  {
    vtkErrorMacro(<< "No points to build");
    return;
  }
  // Prepare
  this->FreeSearchStructure();

  vtkPointSet* pointSet = vtkPointSet::SafeDownCast(this->GetDataSet());
  if (!pointSet)
  {
    vtkErrorMacro("vtkKdTreePointLocator requires a PointSet to build locator.");
    return;
  }
  this->KdTree = vtkKdTree::New();
  this->KdTree->SetUseExistingSearchStructure(this->UseExistingSearchStructure);
  this->KdTree->BuildLocatorFromPoints(pointSet);
  this->KdTree->GetBounds(this->Bounds);
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  this->KdTree->GenerateRepresentation(level, pd);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "KdTree " << this->KdTree << "\n";
}
