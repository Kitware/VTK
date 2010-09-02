/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayVariants.cxx

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

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int TestArrayVariants(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Exercise the API that gets/sets variants ...
    vtkSmartPointer<vtkDenseArray<double> > concrete = vtkSmartPointer<vtkDenseArray<double> >::New();
    concrete->Resize(3, 2);
    vtkTypedArray<double>* const typed = concrete;
    vtkArray* const abstract = concrete;

    abstract->SetVariantValue(0, 0, 1.0);
    abstract->SetVariantValue(vtkArrayCoordinates(0, 1), 2.0);
    typed->SetVariantValue(1, 0, 3.0);
    typed->SetVariantValue(vtkArrayCoordinates(1, 1), 4.0);
    concrete->SetVariantValue(2, 0, 5.0);
    concrete->SetVariantValue(vtkArrayCoordinates(2, 1), 6.0);

    test_expression(abstract->GetVariantValue(0, 0) == 1.0);
    test_expression(abstract->GetVariantValue(vtkArrayCoordinates(0, 1)) == 2.0);
    test_expression(typed->GetVariantValue(1, 0) == 3.0);
    test_expression(typed->GetVariantValue(vtkArrayCoordinates(1, 1)) == 4.0);
    test_expression(concrete->GetVariantValue(2, 0) == 5.0);
    test_expression(concrete->GetVariantValue(vtkArrayCoordinates(2, 1)) == 6.0);

    abstract->SetVariantValueN(0, 7.0);
    test_expression(abstract->GetVariantValueN(0) == 7.0);
    typed->SetVariantValueN(0, 8.0);
    test_expression(typed->GetVariantValueN(0) == 8.0);
    concrete->SetVariantValueN(0, 9.0);
    test_expression(concrete->GetVariantValueN(0) == 9.0);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
