/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSort.h

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

// .NAME vtkSort - Provides several methods for sorting vtk arrays.

#ifndef __vtkSort_h
#define __vtkSort_h

#include "vtkObject.h"

class vtkIdList;
class vtkDataArray;

class VTK_COMMON_EXPORT vtkSort : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSort, vtkObject);
  static vtkSort *New();
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
  vtkSort();
  virtual ~vtkSort();

private:
  vtkSort(const vtkSort &);  // Not implemented.
  void operator=(const vtkSort &);  // Not implemented.
};

#endif //__vtkSort_h
