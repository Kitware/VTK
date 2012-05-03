/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SparseArrayValidation.cxx

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

#include <vtkArrayPrint.h>
#include <vtkSparseArray.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw std::runtime_error("Expression failed: " #expression); \
}

int TestSparseArrayValidation(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create an array ...
    vtkSmartPointer<vtkSparseArray<double> > array = vtkSmartPointer<vtkSparseArray<double> >::New();
    test_expression(array->Validate());

    array->Resize(vtkArrayExtents::Uniform(2, 3));
    test_expression(array->Validate());

    array->Clear();
    array->AddValue(0, 0, 1);
    array->AddValue(1, 2, 2);
    array->AddValue(0, 1, 3);
    test_expression(array->Validate());

    array->Clear();
    array->AddValue(0, 0, 1);
    array->AddValue(1, 2, 2);
    array->AddValue(0, 0, 4);
    test_expression(!array->Validate());

    array->Clear();
    array->AddValue(0, 0, 1);
    array->AddValue(3, 3, 2);
    test_expression(!array->Validate());

    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
