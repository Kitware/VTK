/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSelectArraySlices.cxx
  
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
#include <vtkIdTypeArray.h>
#include <vtkSelectArraySlices.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer;                                       \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

void TestOutput(vtkSelectionAlgorithm* Algorithm, const vtkIdType* const ValueBegin, const vtkIdType* const ValueEnd)
{
  Algorithm->Update();
  vtkSelection* const selection = Algorithm->GetOutput();
  test_expression(selection);

  test_expression(selection->GetNumberOfNodes() == 1);
  vtkSelectionNode* const selection_node = selection->GetNode(0);
  test_expression(selection_node);

  vtkIdTypeArray* const selection_list = vtkIdTypeArray::SafeDownCast(selection_node->GetSelectionList());
  test_expression(selection_list);

  test_expression(selection_list->GetNumberOfTuples() == (ValueEnd - ValueBegin));
  for(const vtkIdType* value = ValueBegin; value != ValueEnd; ++value)
    test_expression(selection_list->GetValue(value - ValueBegin) == (*value));
}

int TestSelectArraySlices(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkSparseArray<double> > array = vtkSmartPointer<vtkSparseArray<double> >::New();
    array->Resize(4, 5);
    array->AddValue(1, 1, 1);
    array->AddValue(2, 2, 2);
    array->AddValue(3, 2, 3);
    array->AddValue(1, 4, 4);
    array->AddValue(2, 4, 5);
    array->AddValue(3, 4, 6);

    cout << "Input array:\n";
    vtkPrintMatrixFormat(cout, array.GetPointer());

    vtkSmartPointer<vtkArrayData> array_data = vtkSmartPointer<vtkArrayData>::New();
    array_data->AddArray(array);

    vtkSmartPointer<vtkSelectArraySlices> select_slices = vtkSmartPointer<vtkSelectArraySlices>::New();
    select_slices->SetInputConnection(0, array_data->GetProducerPort());
    select_slices->SetSliceDimension(1);

    select_slices->SetMinimumCount(1);
    select_slices->SetMinimumPercent(0);
    select_slices->SetMaximumCount(100);
    select_slices->SetMaximumPercent(1);
    vtkIdType a[] = { 1, 2, 4 };
    TestOutput(select_slices, a, a + (sizeof(a) / sizeof(vtkIdType)));
   
    select_slices->SetMinimumCount(2);
    vtkIdType b[] = { 2, 4 };
    TestOutput(select_slices, b, b + (sizeof(b) / sizeof(vtkIdType)));

    select_slices->SetMinimumCount(0);
    select_slices->SetMinimumPercent(0.5);
    vtkIdType c[] = { 2, 4 };
    TestOutput(select_slices, c, c + (sizeof(c) / sizeof(vtkIdType)));

    select_slices->SetMaximumPercent(0.6);
    vtkIdType d[] = { 2 };
    TestOutput(select_slices, d, d + (sizeof(d) / sizeof(vtkIdType)));

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

