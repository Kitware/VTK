/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibilitySort.h

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

/**
 * @class   vtkVisibilitySort
 * @brief   Abstract class that can sort cell data along a viewpoint.
 *
 *
 * vtkVisibilitySort encapsulates a method for depth sorting the cells of a
 * vtkDataSet for a given viewpoint.  It should be noted that subclasses
 * are not required to give an absolutely correct sorting.  Many types of
 * unstructured grids may have sorting cycles, meaning that there is no
 * possible correct sorting.  Some subclasses also only give an approximate
 * sorting in the interest of speed.
 *
 * @attention
 * The Input field of this class tends to causes reference cycles.  To help
 * break these cycles, garbage collection is enabled on this object and the
 * input parameter is traced.  For this to work, though, an object in the
 * loop holding the visibility sort should also report that to the garbage
 * collector.
 *
*/

#ifndef vtkVisibilitySort_h
#define vtkVisibilitySort_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkIdTypeArray;
class vtkDataSet;
class vtkMatrix4x4;
class vtkCamera;

class VTKRENDERINGCORE_EXPORT vtkVisibilitySort : public vtkObject
{
public:
  vtkTypeMacro(vtkVisibilitySort, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * To facilitate incremental sorting algorithms, the cells are retrieved
   * in an iteration process.  That is, call InitTraversal to start the
   * iteration and call GetNextCells to get the cell IDs in order.
   * However, for efficiencies sake, GetNextCells returns an ordered list
   * of several id's in once call (but not necessarily all).  GetNextCells
   * will return NULL once the entire sorted list is output.  The
   * vtkIdTypeArray returned from GetNextCells is a cached array, so do not
   * delete it.  At the same note, do not expect the array to be valid
   * after subsequent calls to GetNextCells.
   */
  virtual void InitTraversal() = 0;
  virtual vtkIdTypeArray *GetNextCells() = 0;
  //@}

  //@{
  /**
   * Set/Get the maximum number of cells that GetNextCells will return
   * in one invocation.
   */
  vtkSetClampMacro(MaxCellsReturned, int, 1, VTK_INT_MAX);
  vtkGetMacro(MaxCellsReturned, int);
  //@}

  //@{
  /**
   * Set/Get the matrix that transforms from object space to world space.
   * Generally, you get this matrix from a call to GetMatrix of a vtkProp3D
   * (vtkActor).
   */
  virtual void SetModelTransform(vtkMatrix4x4 *mat);
  vtkGetObjectMacro(ModelTransform, vtkMatrix4x4);
  //@}

  vtkGetObjectMacro(InverseModelTransform, vtkMatrix4x4);

  //@{
  /**
   * Set/Get the camera that specifies the viewing parameters.
   */
  virtual void SetCamera(vtkCamera *camera);
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Set/Get the data set containing the cells to sort.
   */
  virtual void SetInput(vtkDataSet *data);
  vtkGetObjectMacro(Input, vtkDataSet);
  //@}

  //@{
  /**
   * Set/Get the sorting direction.  Be default, the direction is set
   * to back to front.
   */
  vtkGetMacro(Direction, int);
  vtkSetMacro(Direction, int);
  void SetDirectionToBackToFront() { this->SetDirection(BACK_TO_FRONT); }
  void SetDirectionToFrontToBack() { this->SetDirection(FRONT_TO_BACK); }
  //@}

  enum { BACK_TO_FRONT, FRONT_TO_BACK };

  //@{
  /**
   * Overwritten to enable garbage collection.
   */
  void Register(vtkObjectBase *o) VTK_OVERRIDE;
  void UnRegister(vtkObjectBase *o) VTK_OVERRIDE;
  //@}

protected:
  vtkVisibilitySort();
  ~vtkVisibilitySort() VTK_OVERRIDE;

  vtkTimeStamp LastSortTime;

  vtkMatrix4x4 *ModelTransform;
  vtkMatrix4x4 *InverseModelTransform;
  vtkCamera *Camera;
  vtkDataSet *Input;

  int MaxCellsReturned;

  int Direction;

  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

private:
  vtkVisibilitySort(const vtkVisibilitySort &) VTK_DELETE_FUNCTION;
  void operator=(const vtkVisibilitySort &) VTK_DELETE_FUNCTION;
};

#endif //vtkVisibilitySort_h

