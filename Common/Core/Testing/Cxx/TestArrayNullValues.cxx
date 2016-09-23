/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayNullValues.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSparseArray.h>
#include <vtkSmartPointer.h>

#include <iostream>
#include <stdexcept>

template<typename T>
void VerifyType(const T& DefaultNull, const T& AlternateNull)
{
  // Create a sparse array ...
  vtkSmartPointer<vtkSparseArray<T> > array = vtkSmartPointer<vtkSparseArray<T> >::New();
  array->Resize(2);

  // Verify that the default NULL value is iniitialized correctly ...
  if(array->GetNullValue() != DefaultNull)
  {
    throw std::runtime_error("Incorrect default NULL value for " + std::string(array->GetClassName()));
  }

  // Verify that GetValue() returns the default NULL value for NULL elements ...
  if(array->GetValue(1) != DefaultNull)
  {
    throw std::runtime_error("Empty value did not return default NULL for " + std::string(array->GetClassName()));
  }

  // Verify that we can override the default NULL value ...
  array->SetNullValue(AlternateNull);
  if(array->GetNullValue() != AlternateNull)
  {
    throw std::runtime_error("Error overriding NULL value for " + std::string(array->GetClassName()));
  }

  // Verify that GetValue() returns the alternate NULL value forr NULL elements ...
  if(array->GetValue(1) != AlternateNull)
  {
    throw std::runtime_error("Empty value did not overridden NULL for " + std::string(array->GetClassName()));
  }
}

int TestArrayNullValues(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
  {
    VerifyType<vtkTypeInt8>(0, 1);
    VerifyType<vtkTypeUInt8>(0, 1);
    VerifyType<vtkTypeInt16>(0, 1);
    VerifyType<vtkTypeUInt16>(0, 1);
    VerifyType<vtkTypeInt32>(0, 1);
    VerifyType<vtkTypeUInt32>(0, 1);
    VerifyType<vtkTypeFloat32>(0.0f, 1);
    VerifyType<vtkTypeFloat64>(0.0, 1);
    VerifyType<vtkIdType>(0, 1);
    VerifyType<vtkStdString>(vtkStdString(""), vtkStdString("foo"));

    return 0;
  }
  catch(std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
