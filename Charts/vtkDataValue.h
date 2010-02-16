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


// Warning: we cannot initialize the Constant member in the constructor.
// This is postpone to specialized classes.
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
  virtual T GetConstant()
    {
    return this->Constant;
    }
  virtual FunctionType GetFunction()
    { return this->Function; }

protected:
  T Constant;
  FunctionType Function;
};

// Specialized template for double.
template <>
class vtkValue<double>
{
public:
  typedef double (*FunctionType)(vtkMark*, vtkDataElement&);
  vtkValue() : Constant(0.0), Function(NULL) { }
  vtkValue(FunctionType f) : Constant(0.0), Function(f) { }
  vtkValue(double v) : Constant(v), Function(NULL) { }
  bool IsConstant()
    { return this->Function == NULL; }
  double GetConstant()
    {
      return this->Constant;
    }
  FunctionType GetFunction()
    { return this->Function; }
protected:
  double Constant;
  FunctionType Function;
};


class VTK_CHARTS_EXPORT vtkDataValue : public vtkValue<vtkDataElement>
{
public:
  vtkDataValue()
    {
      this->Constant=vtkDataElement();
      this->Function = NULL;
    }
  vtkDataValue(FunctionType f)
    {
      this->Constant=vtkDataElement();
      this->Function = f;
    }
  vtkDataValue(vtkDataElement v)
    {
      this->Constant = v;
      this->Function = NULL;
    }
  vtkDataElement GetData(vtkMark* m);
};

#endif // __vtkDataValue_h
