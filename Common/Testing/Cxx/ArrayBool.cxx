/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayBool.cxx
  
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

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int ArrayBool(int argc, char* argv[])
{
  try
    {
    // Confirm that we can work with dense arrays of bool values
    vtkSmartPointer<vtkDenseArray<char> > dense = vtkSmartPointer<vtkDenseArray<char> >::New();
    dense->Resize(2, 2);
    dense->Fill(false);

    test_expression(dense->GetValue(1, 1) == false);
    dense->SetValue(1, 1, true);
    test_expression(dense->GetValue(1, 1) == true);
    
    test_expression(dense->GetValue(0, 1) == false);
    (*dense)[vtkArrayCoordinates(0, 1)] = true;
    test_expression(dense->GetValue(0, 1) == true);

    // Confirm that we can work with sparse arrays of bool values
    vtkSmartPointer<vtkSparseArray<char> > sparse = vtkSmartPointer<vtkSparseArray<char> >::New();
    sparse->Resize(2, 2);

    test_expression(sparse->GetValue(1, 1) == false);
    sparse->SetValue(1, 1, true);
    test_expression(sparse->GetValue(1, 1) == true);
    
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

