/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMatrixWeighting.cxx
  
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

#include <vtkUnityMatrixWeighting.h>
#include <vtkEntropyMatrixWeighting.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>
#include <vtkDenseArray.h>

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

int TestMatrixWeighting(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkArrayData> a_data = vtkSmartPointer<vtkArrayData>::New();
    vtkSmartPointer<vtkArrayData> b_data = vtkSmartPointer<vtkArrayData>::New();

    vtkSmartPointer<vtkSparseArray<double> > a = vtkSmartPointer<vtkSparseArray<double> >::New();
    vtkSmartPointer<vtkDenseArray<double> > b = vtkSmartPointer<vtkDenseArray<double> >::New();

    a->Resize(10, 15);
    a->SetValue(0, 0, 1);
    a->SetValue(1, 1, 2);
    
    b->Resize(10, 15);
    b->Fill(1.0);

    a_data->AddArray(a);
    b_data->AddArray(b);

    vtkSmartPointer<vtkUnityMatrixWeighting> unity_weighting = vtkSmartPointer<vtkUnityMatrixWeighting>::New();
    vtkSmartPointer<vtkEntropyMatrixWeighting> entropy_weighting = vtkSmartPointer<vtkEntropyMatrixWeighting>::New();

    unity_weighting->SetInputConnection(0, a_data->GetProducerPort());
    entropy_weighting->SetFeatureDimension(1);
    entropy_weighting->SetInputConnection(0, b_data->GetProducerPort());
    unity_weighting->Update();
    entropy_weighting->Update();

    vtkDenseArray<double>* const unity = vtkDenseArray<double>::SafeDownCast(unity_weighting->GetOutput()->GetArray(0));
    vtkDenseArray<double>* const entropy = vtkDenseArray<double>::SafeDownCast(entropy_weighting->GetOutput()->GetArray(0));

    test_expression(unity->GetExtent(0).GetSize() == 10);
    for(int i = unity->GetExtent(0).GetBegin(); i<unity->GetExtent(0).GetEnd(); i++)
      {
      test_expression(int(unity->GetValue(i)) == 1);
      }

    test_expression(entropy->GetExtent(0).GetSize()  == 15);
    for(int i = entropy->GetExtent(0).GetBegin(); i<entropy->GetExtent(0).GetEnd(); i++)
      {
      test_expression(entropy->GetValue(i) < 0.00001);
      }

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

