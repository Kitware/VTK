/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenterDepthSort.h

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

// .NAME vtkCellCenterDepthSort - A simple implementation of vtkCellDepthSort.
//
// .SECTION Description
// vtkCellCenterDepthSort is a simple and fast implementation of depth
// sort, but it only provides approximate results.  The sorting algorithm
// finds the centroids of all the cells.  It then performs the dot product
// of the centroids against a vector pointing in the direction of the
// camera transformed into object space.  It then performs an ordinary sort
// on the result.
//

#ifndef __vtkCellCenterDepthSort_h
#define __vtkCellCenterDepthSort_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkVisibilitySort.h"

class vtkFloatArray;

class vtkCellCenterDepthSortStack;

class VTKRENDERINGCORE_EXPORT vtkCellCenterDepthSort : public vtkVisibilitySort
{
public:
  vtkTypeMacro(vtkCellCenterDepthSort, vtkVisibilitySort);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkCellCenterDepthSort *New();

  virtual void InitTraversal();
  virtual vtkIdTypeArray *GetNextCells();

protected:
  vtkCellCenterDepthSort();
  virtual ~vtkCellCenterDepthSort();

  vtkIdTypeArray *SortedCells;
  vtkIdTypeArray *SortedCellPartition;

  vtkFloatArray *CellCenters;
  vtkFloatArray *CellDepths;
  vtkFloatArray *CellPartitionDepths;

  virtual float *ComputeProjectionVector();
  virtual void ComputeCellCenters();
  virtual void ComputeDepths();

private:
  vtkCellCenterDepthSortStack *ToSort;

  vtkCellCenterDepthSort(const vtkCellCenterDepthSort &);  // Not implemented.
  void operator=(const vtkCellCenterDepthSort &);  // Not implemented.
};

#endif
