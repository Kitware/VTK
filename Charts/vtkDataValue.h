/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataValue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDataValue - Class to hold the data
// (as function or vtkDataElement).
//
// .SECTION Description
//

#ifndef __vtkDataValue_h
#define __vtkDataValue_h

#include "vtkDataElement.h"

// Forware declaration.
class vtkMark;

template <typename T>
class vtkValue
{
public:
  typedef T (*FunctionType)(vtkMark*, vtkDataElement&);
  vtkValue() : Function(NULL) { }
  vtkValue(FunctionType f) : Function(f) { }
  vtkValue(T v) : Constant(v), Function(NULL) { }
  bool IsConstant()
    { return this->Function == NULL; }
  T GetConstant()
    {
    return this->Constant;
    }
  FunctionType GetFunction()
    { return this->Function; }

protected:
  T Constant;
  FunctionType Function;
};

class VTK_CHARTS_EXPORT vtkDataValue : public vtkValue<vtkDataElement>
{
public:
  vtkDataValue() { this->Function = NULL; }
  vtkDataValue(FunctionType f)  { this->Function = f; }
  vtkDataValue(vtkDataElement v) { this->Constant = v; this->Function = NULL; }
  vtkDataElement GetData(vtkMark* m);
};

#endif // __vtkDataValue_h
