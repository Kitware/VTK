/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectedSlices.cxx
  
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
#include <vtkExtractSelectedRows.h>
#include <vtkExtractSelectedSlices.h>
#include <vtkIdTypeArray.h>
#include <vtkSelectArraySlices.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>
#include <vtkTable.h>

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

int TestExtractSelectedSlices(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    // Create a sample document dictionary and frequency matrix ...
    vtkIdTypeArray* const documents = vtkIdTypeArray::New();
    documents->SetName("document");
    documents->InsertNextValue(0);
    documents->InsertNextValue(1);
    documents->InsertNextValue(2);
    documents->InsertNextValue(3);
    documents->InsertNextValue(4);

    vtkSmartPointer<vtkTable> input_dictionary = vtkSmartPointer<vtkTable>::New();
    input_dictionary->AddColumn(documents);
    documents->Delete();

    cout << "Input dictionary:\n";
    input_dictionary->Dump(20);    
    cout << "\n";
      
    vtkSmartPointer<vtkSparseArray<double> > array = vtkSmartPointer<vtkSparseArray<double> >::New();
    array->Resize(4, input_dictionary->GetNumberOfRows());
    array->AddValue(1, 1, 1);
    array->AddValue(2, 2, 2);
    array->AddValue(3, 2, 3);
    array->AddValue(0, 4, 4);
    array->AddValue(1, 4, 5);
    array->AddValue(2, 4, 6);

    cout << "Input frequency matrix:\n";
    vtkPrintMatrixFormat(cout, array.GetPointer());
    cout << "\n";

    vtkSmartPointer<vtkArrayData> input_frequency_matrix = vtkSmartPointer<vtkArrayData>::New();
    input_frequency_matrix->AddArray(array);

    // Setup the pipeline to remove empty columns from the frequency matrix and matching rows from the dictionary ...
    vtkSmartPointer<vtkSelectArraySlices> select_slices = vtkSmartPointer<vtkSelectArraySlices>::New();
    select_slices->SetInputConnection(0, input_frequency_matrix->GetProducerPort());
    select_slices->SetSliceDimension(1);
    select_slices->SetMinimumCount(1);
    select_slices->SetMinimumPercent(0);
    select_slices->SetMaximumCount(100);
    select_slices->SetMaximumPercent(1);

    vtkSmartPointer<vtkExtractSelectedRows> extract_rows = vtkSmartPointer<vtkExtractSelectedRows>::New();
    extract_rows->SetInputConnection(0, input_dictionary->GetProducerPort());
    extract_rows->SetInputConnection(1, select_slices->GetOutputPort());

    vtkSmartPointer<vtkExtractSelectedSlices> extract_slices = vtkSmartPointer<vtkExtractSelectedSlices>::New();
    extract_slices->SetInputConnection(0, input_frequency_matrix->GetProducerPort());
    extract_slices->SetInputConnection(1, select_slices->GetOutputPort());
    extract_slices->SetSliceDimension(1);

    // Test the results ...
    extract_rows->Update();
    vtkTable* const output_dictionary = extract_rows->GetOutput();
    test_expression(output_dictionary);

    cout << "Output dictionary:\n";
    output_dictionary->Dump(20);
    cout << "\n";
   
    test_expression(output_dictionary->GetNumberOfRows() == 3);
    test_expression(output_dictionary->GetValueByName(0, "document").ToInt() == 1);
    test_expression(output_dictionary->GetValueByName(1, "document").ToInt() == 2);
    test_expression(output_dictionary->GetValueByName(2, "document").ToInt() == 4);
    
    extract_slices->Update();
    test_expression(extract_slices->GetOutput());
    test_expression(extract_slices->GetOutput()->GetNumberOfArrays() == 1);
    test_expression(extract_slices->GetOutput()->GetArray(0));

    vtkTypedArray<double>* const output_frequency_matrix = vtkTypedArray<double>::SafeDownCast(extract_slices->GetOutput()->GetArray(0));
    test_expression(output_frequency_matrix);

    cout << "Output frequency matrix:\n";
    vtkPrintMatrixFormat(cout, output_frequency_matrix);
    cout << "\n";

    test_expression(output_frequency_matrix->GetExtent(0).GetSize() == 4);
    test_expression(output_frequency_matrix->GetExtent(1).GetSize() == 3);
    test_expression(output_frequency_matrix->GetValue(1, 0) == 1);
    test_expression(output_frequency_matrix->GetValue(2, 1) == 2);
    test_expression(output_frequency_matrix->GetValue(3, 1) == 3);
    test_expression(output_frequency_matrix->GetValue(0, 2) == 4);
    test_expression(output_frequency_matrix->GetValue(1, 2) == 5);
    test_expression(output_frequency_matrix->GetValue(2, 2) == 6);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

