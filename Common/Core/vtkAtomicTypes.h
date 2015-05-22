/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkAtomicTypes_h
#define vtkAtomicTypes_h

#include "vtkAtomic.h"
#include "vtkType.h"

typedef vtkAtomic<vtkTypeInt32> vtkAtomicInt32;
typedef vtkAtomic<vtkTypeUInt32> vtkAtomicUInt32;
typedef vtkAtomic<vtkTypeInt64> vtkAtomicInt64;
typedef vtkAtomic<vtkTypeUInt64> vtkAtomicUInt64;
typedef vtkAtomic<vtkIdType> vtkAtomicIdType;

#endif
// VTK-HeaderTest-Exclude: vtkAtomicTypes.h
