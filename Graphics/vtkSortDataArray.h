/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortDataArray.h

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

// .NAME vtkSortDataArray - Provides several methods for sorting vtk arrays.

#ifndef __vtkSortDataArray_h
#define __vtkSortDataArray_h

#include "vtkObject.h"

class vtkIdList;
class vtkDataArray;

class VTK_GRAPHICS_EXPORT vtkSortDataArray : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSortDataArray, vtkObject);
  static vtkSortDataArray *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Sorts the given array.
  static void Sort(vtkIdList *keys);
  static void Sort(vtkDataArray *keys);

  // Description:
  // Sorts the given key/value pairs based on the keys.  A pair is given
  // as the entries at a given index of each of the arrays.  Obviously,
  // the two arrays must be of equal size.
  static void Sort(vtkIdList *keys, vtkIdList *values);
  static void Sort(vtkIdList *keys, vtkDataArray *values);
  static void Sort(vtkDataArray *keys, vtkIdList *values);
  static void Sort(vtkDataArray *keys, vtkDataArray *values);

protected:
  vtkSortDataArray();
  virtual ~vtkSortDataArray();

private:
  vtkSortDataArray(const vtkSortDataArray &);  // Not implemented.
  void operator=(const vtkSortDataArray &);  // Not implemented.
};

#endif //__vtkSortDataArray_h
