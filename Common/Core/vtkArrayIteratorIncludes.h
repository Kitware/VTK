/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayIteratorIncludes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArrayIteratorIncludes - centralize array iterator type includes
// required when using the vtkArrayIteratorTemplateMacro.
//
// .SECTION Description
// A CXX file using vtkArrayIteratorTemplateMacro needs to include the
// header files for all types of iterators supported by the macro.  As
// new arrays and new iterators are added, vtkArrayIteratorTemplateMacro
// will also need to be updated to switch to the additional cases.
// However, this would imply any code using the macro will start giving
// compilation errors unless they include the new iterator headers. The
// vtkArrayIteratorIncludes.h will streamline this issue. Every file
// using the vtkArrayIteratorTemplateMacro must include this
// vtkArrayIteratorIncludes.h. As new iterators are added and the
// vtkArrayIteratorTemplateMacro updated, one needs to update this header
// file alone.

#ifndef __vtkArrayIteratorIncludes_h
#define __vtkArrayIteratorIncludes_h

// Iterators.
#include "vtkArrayIteratorTemplate.h"
#include "vtkBitArrayIterator.h"

// DataTypes for Iterators.
#include "vtkStdString.h"

#endif

// VTK-HeaderTest-Exclude: vtkArrayIteratorIncludes.h
