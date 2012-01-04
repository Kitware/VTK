/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointwiseMutualInformation.cxx
  
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
#include <vtkConcatenateArray.h>
#include <vtkDenseArray.h>
#include <vtkPointwiseMutualInformation.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#include <cmath>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

static bool close_enough(const double& lhs, const double& rhs)
{
    return std::fabs(lhs - rhs) < 1.0e-10;
}

int TestPointwiseMutualInformation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<double> > a = vtkSmartPointer<vtkDenseArray<double> >::New();

    a->Resize(2, 2);
    a->SetValue(0, 0, 0);
    a->SetValue(0, 1, 1);
    a->SetValue(1, 0, 1);
    a->SetValue(1, 1, 1);
   
    cerr << "input matrix: " << endl; 
    vtkPrintMatrixFormat(cerr, a.GetPointer());

    vtkSmartPointer<vtkArrayData> a_data = vtkSmartPointer<vtkArrayData>::New();
    a_data->AddArray(a);

    vtkSmartPointer<vtkPointwiseMutualInformation> pmi = vtkSmartPointer<vtkPointwiseMutualInformation>::New();
    pmi->SetInputConnection(0, a_data->GetProducerPort());
    pmi->Update();

    test_expression(pmi->GetOutput());
    vtkTypedArray<double>* const array = vtkTypedArray<double>::SafeDownCast(pmi->GetOutput()->GetArray(0));
    test_expression(array);
  
    cerr << "output matrix: " << endl; 
    vtkPrintMatrixFormat(cerr, array);

    test_expression(a->GetExtents() == array->GetExtents());

    test_expression(close_enough(array->GetValue(0, 0), 0));
    test_expression(close_enough(array->GetValue(0, 1), std::log(2.0 / 3.0) / std::log(2.0)));
    test_expression(close_enough(array->GetValue(1, 0), std::log(2.0 / 3.0) / std::log(2.0)));
    test_expression(close_enough(array->GetValue(1, 1), std::log(4.0 / 3.0) / std::log(2.0)));
 
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

