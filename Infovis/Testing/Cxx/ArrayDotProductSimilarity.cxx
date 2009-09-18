/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayDotProductSimilarity.cxx
  
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

#include <vtkArrayData.h>
#include <vtkArrayPrint.h>
#include <vtkDotProductSimilarity.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

#if 0
static bool close_enough(const double lhs, const double rhs)
{
  return fabs(lhs - rhs) < 1.0e-12;
}
#endif

int ArrayDotProductSimilarity(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  cout << setprecision(17);
  
  try
    {
    // Run tests on one matrix ...
    vtkSmartPointer<vtkDenseArray<double> > matrix_a = vtkSmartPointer<vtkDenseArray<double> >::New();
    matrix_a->Resize(vtkArrayExtents(2, 2));
    matrix_a->SetValue(vtkArrayCoordinates(0, 0), 1);
    matrix_a->SetValue(vtkArrayCoordinates(1, 0), 2);
    matrix_a->SetValue(vtkArrayCoordinates(0, 1), 3);
    matrix_a->SetValue(vtkArrayCoordinates(1, 1), 4);

    vtkSmartPointer<vtkArrayData> matrix_data_a = vtkSmartPointer<vtkArrayData>::New();
    matrix_data_a->AddArray(matrix_a);
     
    vtkSmartPointer<vtkDotProductSimilarity> similarity = vtkSmartPointer<vtkDotProductSimilarity>::New();
    similarity->SetInputConnection(0, matrix_data_a->GetProducerPort());
    similarity->SetVectorDimension(1);
    similarity->SetMinimumThreshold(0);
    similarity->SetMinimumCount(0);

    similarity->SetUpperDiagonal(true);
    similarity->SetDiagonal(false);
    similarity->SetLowerDiagonal(false);
    similarity->Update();
    similarity->GetOutput()->Dump(10);

    test_expression(similarity->GetOutput()->GetNumberOfRows() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(0, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(0, "similarity").ToInt() == 11);
    
    similarity->SetUpperDiagonal(false);
    similarity->SetDiagonal(true);
    similarity->SetLowerDiagonal(false);
    similarity->Update();
    similarity->GetOutput()->Dump(10);
    test_expression(similarity->GetOutput()->GetValueByName(0, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "similarity").ToInt() == 5);
    test_expression(similarity->GetOutput()->GetValueByName(1, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(1, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(1, "similarity").ToInt() == 25);
    
    test_expression(similarity->GetOutput()->GetNumberOfRows() == 2);
    
    similarity->SetUpperDiagonal(false);
    similarity->SetDiagonal(false);
    similarity->SetLowerDiagonal(true);
    similarity->Update();
    similarity->GetOutput()->Dump(10);
    
    test_expression(similarity->GetOutput()->GetNumberOfRows() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(0, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(0, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "similarity").ToInt() == 11);
    
    // Run tests with two matrices ...
    vtkSmartPointer<vtkDenseArray<double> > matrix_b = vtkSmartPointer<vtkDenseArray<double> >::New();
    matrix_b->Resize(vtkArrayExtents(2, 2));
    matrix_b->SetValue(vtkArrayCoordinates(0, 0), 5);
    matrix_b->SetValue(vtkArrayCoordinates(1, 0), 6);
    matrix_b->SetValue(vtkArrayCoordinates(0, 1), 7);
    matrix_b->SetValue(vtkArrayCoordinates(1, 1), 8);

    vtkSmartPointer<vtkArrayData> matrix_data_b = vtkSmartPointer<vtkArrayData>::New();
    matrix_data_b->AddArray(matrix_b);

    similarity->SetInputConnection(1, matrix_data_b->GetProducerPort());

    similarity->SetFirstSecond(true);
    similarity->SetSecondFirst(false);
    similarity->Update();
    similarity->GetOutput()->Dump(10);

    test_expression(similarity->GetOutput()->GetNumberOfRows() == 4);
    test_expression(similarity->GetOutput()->GetValueByName(0, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "similarity").ToInt() == 17);
    test_expression(similarity->GetOutput()->GetValueByName(1, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(1, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(1, "similarity").ToInt() == 23);
    test_expression(similarity->GetOutput()->GetValueByName(2, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(2, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(2, "similarity").ToInt() == 39);
    test_expression(similarity->GetOutput()->GetValueByName(3, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(3, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(3, "similarity").ToInt() == 53);
 
    similarity->SetFirstSecond(false);
    similarity->SetSecondFirst(true);
    similarity->Update();
    similarity->GetOutput()->Dump(10);
 
    test_expression(similarity->GetOutput()->GetNumberOfRows() == 4);
    test_expression(similarity->GetOutput()->GetValueByName(0, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(0, "similarity").ToInt() == 17);
    test_expression(similarity->GetOutput()->GetValueByName(1, "source").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(1, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(1, "similarity").ToInt() == 39);
    test_expression(similarity->GetOutput()->GetValueByName(2, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(2, "target").ToInt() == 0);
    test_expression(similarity->GetOutput()->GetValueByName(2, "similarity").ToInt() == 23);
    test_expression(similarity->GetOutput()->GetValueByName(3, "source").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(3, "target").ToInt() == 1);
    test_expression(similarity->GetOutput()->GetValueByName(3, "similarity").ToInt() == 53);
 
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

