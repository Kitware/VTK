/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenterDepthSort.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkCellCenterDepthSort.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkDataSet.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkFloatArray.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkSortDataArray.h"

#include <stack>
#include <utility>
#include <algorithm>

//-----------------------------------------------------------------------------

typedef std::pair<vtkIdType, vtkIdType> vtkIdPair;

class vtkCellCenterDepthSortStack
{
public:
  std::stack<vtkIdPair> Stack;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkCellCenterDepthSort);

vtkCellCenterDepthSort::vtkCellCenterDepthSort()
{
  this->SortedCells = vtkIdTypeArray::New();
  this->SortedCells->SetNumberOfComponents(1);
  this->SortedCellPartition = vtkIdTypeArray::New();
  this->SortedCells->SetNumberOfComponents(1);

  this->CellCenters = vtkFloatArray::New();
  this->CellCenters->SetNumberOfComponents(3);
  this->CellDepths = vtkFloatArray::New();
  this->CellDepths->SetNumberOfComponents(1);
  this->CellPartitionDepths = vtkFloatArray::New();
  this->CellPartitionDepths->SetNumberOfComponents(1);

  this->ToSort = new vtkCellCenterDepthSortStack;
}

vtkCellCenterDepthSort::~vtkCellCenterDepthSort()
{
  this->SortedCells->Delete();
  this->SortedCellPartition->Delete();
  this->CellCenters->Delete();
  this->CellDepths->Delete();
  this->CellPartitionDepths->Delete();

  delete this->ToSort;
}

void vtkCellCenterDepthSort::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

float *vtkCellCenterDepthSort::ComputeProjectionVector()
{
  vtkDebugMacro("ComputeProjectionVector");

  if (this->Camera == NULL)
    {
    vtkErrorMacro("Must set camera before sorting cells.");
    static float v[3] = { 0.0, 0.0, 0.0};
    return v;
    }

  double focalPoint[4];
  double position[4];

  this->Camera->GetFocalPoint(focalPoint);  focalPoint[3] = 1.0;
  this->Camera->GetPosition(position);  position[3] = 1.0;

  this->InverseModelTransform->MultiplyPoint(focalPoint, focalPoint);
  this->InverseModelTransform->MultiplyPoint(position, position);

  static float vector[3];
  if (this->Direction == vtkVisibilitySort::BACK_TO_FRONT)
    {
    // Sort back to front.
    vector[0] = position[0] - focalPoint[0];
    vector[1] = position[1] - focalPoint[1];
    vector[2] = position[2] - focalPoint[2];
    }
  else
    {
    // Sort front to back.
    vector[0] = focalPoint[0] - position[0];
    vector[1] = focalPoint[1] - position[1];
    vector[2] = focalPoint[2] - position[2];
    }

  vtkDebugMacro("Returning: " << vector[0] << ", " << vector[1] << ", "
                << vector[2]);

  return vector;
}

void vtkCellCenterDepthSort::ComputeCellCenters()
{
  vtkIdType numcells = this->Input->GetNumberOfCells();
  this->CellCenters->SetNumberOfTuples(numcells);

  float *center = this->CellCenters->GetPointer(0);
  double dcenter[3];
  double *weights = new double[this->Input->GetMaxCellSize()];  //Dummy array.

  for (vtkIdType i = 0; i < numcells; i++)
    {
    vtkCell *cell = this->Input->GetCell(i);
    double pcenter[3];
    int subId;
    subId = cell->GetParametricCenter(pcenter);
    cell->EvaluateLocation(subId, pcenter, dcenter, weights);
    center[0] = dcenter[0]; center[1] = dcenter[1]; center[2] = dcenter[2];
    center += 3;
    }

  delete[] weights;
}

void vtkCellCenterDepthSort::ComputeDepths()
{
  float *vector = this->ComputeProjectionVector();
  vtkIdType numcells = this->Input->GetNumberOfCells();

  float *center = this->CellCenters->GetPointer(0);
  float *depth = this->CellDepths->GetPointer(0);
  for (vtkIdType i = 0; i < numcells; i++)
    {
    *(depth++) = vtkMath::Dot(center, vector);
    center += 3;
    }
}

void vtkCellCenterDepthSort::InitTraversal()
{
  vtkDebugMacro("InitTraversal");

  vtkIdType numcells = this->Input->GetNumberOfCells();

  if (   (this->LastSortTime < this->Input->GetMTime())
      || (this->LastSortTime < this->MTime) )
    {
    vtkDebugMacro("Building cell centers array.");

    // Data may have changed.  Recompute cell centers.
    this->ComputeCellCenters();
    this->CellDepths->SetNumberOfTuples(numcells);
    this->SortedCells->SetNumberOfTuples(numcells);
    }

  vtkDebugMacro("Filling SortedCells to initial values.");
  vtkIdType *id = this->SortedCells->GetPointer(0);
  for (vtkIdType i = 0; i < numcells; i++)
    {
    *(id++) = i;
    }

  vtkDebugMacro("Calculating depths.");
  this->ComputeDepths();

  while (!this->ToSort->Stack.empty()) this->ToSort->Stack.pop();
  this->ToSort->Stack.push(vtkIdPair(0, numcells));

  this->LastSortTime.Modified();
}

vtkIdTypeArray *vtkCellCenterDepthSort::GetNextCells()
{
  if (this->ToSort->Stack.empty())
    {
    // Already sorted and returned everything.
    return NULL;
    }

  vtkIdType *cellIds = this->SortedCells->GetPointer(0);
  float *cellDepths = this->CellDepths->GetPointer(0);
  vtkIdPair partition;

  partition = this->ToSort->Stack.top();  this->ToSort->Stack.pop();
  while (partition.second - partition.first > this->MaxCellsReturned)
    {
    vtkIdType left = partition.first;
    vtkIdType right = partition.second - 1;
    float pivot = cellDepths[static_cast<vtkIdType>(
                               vtkMath::Random(left, right))];
    while (left <= right)
      {
      while ((left <= right) && (cellDepths[left] < pivot)) left++;
      while ((left <= right) && (cellDepths[right] > pivot)) right--;

      if (left > right) break;

      std::swap(cellIds[left], cellIds[right]);
      std::swap(cellDepths[left], cellDepths[right]);

      left++;  right--;
      }

    this->ToSort->Stack.push(vtkIdPair(left, partition.second));
    partition.second = left;
    }

  if (partition.second <= partition.first)
    {
    // Got a partition of zero size.  Just recurse to get the next one.
    return this->GetNextCells();
    }

  vtkIdType firstcell = partition.first;
  vtkIdType numcells = partition.second - partition.first;

  this->SortedCellPartition->SetArray(cellIds + firstcell, numcells, 1);
  this->SortedCellPartition->SetNumberOfTuples(numcells);
  this->CellPartitionDepths->SetArray(cellDepths + firstcell, numcells, 1);
  this->CellPartitionDepths->SetNumberOfTuples(numcells);

  vtkSortDataArray::Sort(this->CellPartitionDepths, this->SortedCellPartition);
  return this->SortedCellPartition;
}
