/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPI.cxx
  
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

int ArrayAPI(int argc, char* argv[])
{
  try
    {
    vtkSmartPointer<vtkArray> array;

    // Test to see that we can create every supported combination of storage- and value-type.
    vtkstd::vector<int> storage_types;
    storage_types.push_back(vtkArray::DENSE);
    storage_types.push_back(vtkArray::SPARSE);

    vtkstd::vector<int> value_types;
    value_types.push_back(VTK_CHAR);
    value_types.push_back(VTK_UNSIGNED_CHAR);
    value_types.push_back(VTK_SHORT);
    value_types.push_back(VTK_UNSIGNED_SHORT);
    value_types.push_back(VTK_INT);
    value_types.push_back(VTK_UNSIGNED_INT);
    value_types.push_back(VTK_LONG);
    value_types.push_back(VTK_UNSIGNED_LONG);
    value_types.push_back(VTK_DOUBLE);
    value_types.push_back(VTK_ID_TYPE);
    value_types.push_back(VTK_STRING);
    value_types.push_back(VTK_VARIANT);

    for(vtkstd::vector<int>::const_iterator storage_type = storage_types.begin(); storage_type != storage_types.end(); ++storage_type)
      {
      for(vtkstd::vector<int>::const_iterator value_type = value_types.begin(); value_type != value_types.end(); ++value_type)
        {
        cerr << "creating array with storage type " << *storage_type << " and value type " << vtkImageScalarTypeNameMacro(*value_type) << endl;

        array.TakeReference(vtkArray::CreateArray(*storage_type, *value_type));
        test_expression(array);
        }
      }
   
    // Do some spot-checking to see that the actual type matches what we expect ... 
    array.TakeReference(vtkArray::CreateArray(vtkArray::DENSE, VTK_DOUBLE));
    test_expression(vtkDenseArray<double>::SafeDownCast(array));
    
    array.TakeReference(vtkArray::CreateArray(vtkArray::SPARSE, VTK_STRING));
    test_expression(vtkSparseArray<vtkStdString>::SafeDownCast(array));
    
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

