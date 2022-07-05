/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArraySerialization.cxx

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
#include <vtkArrayReader.h>
#include <vtkArrayWriter.h>
#include <vtkDenseArray.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  do                                                                                               \
  {                                                                                                \
    if (!(expression))                                                                             \
    {                                                                                              \
      std::ostringstream buffer;                                                                   \
      buffer << "Expression failed at line " << __LINE__ << ": " << #expression;                   \
      throw std::runtime_error(buffer.str());                                                      \
    }                                                                                              \
  } while (false)

int TestArraySerialization(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
  {
    // Test Read and Write in Ascii text mode
    // Test sparse-array round-trip ...
    vtkSmartPointer<vtkSparseArray<double>> a1 = vtkSmartPointer<vtkSparseArray<double>>::New();
    a1->SetName("a1");
    a1->Resize(2, 2);
    a1->SetDimensionLabel(0, "rows");
    a1->SetDimensionLabel(1, "columns");
    a1->SetNullValue(0.5);
    a1->AddValue(0, 0, 1.5);
    a1->AddValue(1, 1, 2.5);

    std::stringstream a_buffer;
    vtkArrayWriter::Write(a1, a_buffer);

    vtkSmartPointer<vtkArray> a2;
    a2.TakeReference(vtkArrayReader::Read(a_buffer));

    test_expression(a2);
    test_expression(a2->GetName() == "a1");
    test_expression(vtkSparseArray<double>::SafeDownCast(a2));
    test_expression(a2->GetExtents() == a1->GetExtents());
    test_expression(a2->GetNonNullSize() == a1->GetNonNullSize());
    test_expression(a2->GetDimensionLabel(0) == "rows");
    test_expression(a2->GetDimensionLabel(1) == "columns");
    test_expression(vtkSparseArray<double>::SafeDownCast(a2)->GetNullValue() == 0.5);
    test_expression(a2->GetVariantValue(0, 0).ToDouble() == 1.5);
    test_expression(a2->GetVariantValue(0, 1).ToDouble() == 0.5);
    test_expression(a2->GetVariantValue(1, 1).ToDouble() == 2.5);

    // Test sparse-array coordinates out-of-bounds ...
    std::istringstream b_buffer(
      "vtk-sparse-array double\nascii\nb1\n0 2 0 2 1\nrows\ncolumns\n0\n2 2 3.5\n");
    vtkSmartPointer<vtkArray> b1;
    b1.TakeReference(vtkArrayReader::Read(b_buffer));

    test_expression(!b1);

    // Test sparse-array not enough values ...
    std::istringstream d_buffer(
      "vtk-sparse-array double\nascii\nd1\n0 2 0 2 1\nrows\ncolumns\n0\n");
    vtkSmartPointer<vtkArray> d1;
    d1.TakeReference(vtkArrayReader::Read(d_buffer));

    test_expression(!d1);

    // Test dense string arrays containing whitespace ...
    std::istringstream e_buffer(
      "vtk-dense-array string\nascii\ne1\n0 3 3\nvalues\nThe\nquick brown\nfox\n");
    vtkSmartPointer<vtkArray> e1;
    e1.TakeReference(vtkArrayReader::Read(e_buffer));

    test_expression(e1);
    test_expression(vtkDenseArray<vtkStdString>::SafeDownCast(e1));
    test_expression(e1->GetNonNullSize() == 3);
    test_expression(e1->GetVariantValue(0).ToString() == "The");
    test_expression(e1->GetVariantValue(1).ToString() == "quick brown");
    test_expression(e1->GetVariantValue(2).ToString() == "fox");

    // Test sparse string arrays containing whitespace ...
    std::istringstream f_buffer("vtk-sparse-array string\nascii\nf1\n0 3 3\nvalues\nempty value\n0 "
                                "The\n1 quick brown\n2 fox\n");
    vtkSmartPointer<vtkArray> f1;
    f1.TakeReference(vtkArrayReader::Read(f_buffer));

    test_expression(f1);
    test_expression(vtkSparseArray<vtkStdString>::SafeDownCast(f1));
    test_expression(f1->GetNonNullSize() == 3);
    test_expression(
      vtkSparseArray<vtkStdString>::SafeDownCast(f1)->GetNullValue() == "empty value");
    test_expression(f1->GetVariantValue(0).ToString() == "The");
    test_expression(f1->GetVariantValue(1).ToString() == "quick brown");
    test_expression(f1->GetVariantValue(2).ToString() == "fox");

    // Test sparse arrays with DOS line endings ...
    std::istringstream i_buffer(
      "vtk-sparse-array double\r\nascii\r\ni1\r\n0 2 0 2 1\r\nrows\r\ncolumns\r\n0\r\n0 0 5\r\n");
    vtkSmartPointer<vtkArray> i1;
    i1.TakeReference(vtkArrayReader::Read(i_buffer));

    test_expression(i1);
    test_expression(vtkSparseArray<double>::SafeDownCast(i1));
    test_expression(i1->GetNonNullSize() == 1);
    test_expression(i1->GetVariantValue(0, 0).ToDouble() == 5);
    test_expression(i1->GetVariantValue(1, 0).ToDouble() == 0);

    // Test writing to string and reading back ...
    vtkNew<vtkSparseArray<vtkStdString>> j1;
    j1->Resize(3);
    j1->SetNullValue("nothing here");
    j1->SetValue(0, "The");
    j1->SetValue(1, "quick brown");
    j1->SetValue(2, "fox");

    vtkNew<vtkArrayData> j1d;
    j1d->AddArray(j1);

    vtkNew<vtkArrayWriter> jw;
    jw->WriteToOutputStringOn();
    jw->SetInputData(j1d);
    jw->Write();
    std::string js = jw->GetOutputString();

    vtkNew<vtkArrayReader> jr;
    jr->ReadFromInputStringOn();
    jr->SetInputString(js);
    jr->Update();
    vtkArray* j2 = jr->GetOutput()->GetArray(0);

    test_expression(j2);
    test_expression(vtkSparseArray<vtkStdString>::SafeDownCast(j2));
    test_expression(j2->GetNonNullSize() == 3);
    test_expression(
      vtkSparseArray<vtkStdString>::SafeDownCast(j2)->GetNullValue() == "nothing here");
    test_expression(j2->GetVariantValue(0).ToString() == "The");
    test_expression(j2->GetVariantValue(1).ToString() == "quick brown");
    test_expression(j2->GetVariantValue(2).ToString() == "fox");

    // Test Read and Write in Binary mode
    // Test sparse-array round-trip ...
    vtkSmartPointer<vtkSparseArray<double>> ba1 = vtkSmartPointer<vtkSparseArray<double>>::New();
    ba1->SetName("ba1");
    ba1->Resize(2, 2);
    ba1->SetNullValue(0.5);
    ba1->AddValue(0, 0, 1.5);
    ba1->AddValue(1, 1, 2.5);

    std::stringstream ba_buffer;
    vtkArrayWriter::Write(ba1, ba_buffer, true);
    vtkSmartPointer<vtkArray> ba2;
    ba2.TakeReference(vtkArrayReader::Read(ba_buffer));

    test_expression(ba2);
    test_expression(ba2->GetName() == "ba1");
    test_expression(vtkSparseArray<double>::SafeDownCast(ba2));
    test_expression(ba2->GetExtents() == ba1->GetExtents());
    test_expression(ba2->GetNonNullSize() == ba1->GetNonNullSize());
    test_expression(vtkSparseArray<double>::SafeDownCast(ba2)->GetNullValue() == 0.5);
    test_expression(ba2->GetVariantValue(0, 0).ToDouble() == 1.5);
    test_expression(ba2->GetVariantValue(0, 1).ToDouble() == 0.5);
    test_expression(ba2->GetVariantValue(1, 1).ToDouble() == 2.5);

    // Test dense string arrays containing whitespace ...
    vtkSmartPointer<vtkDenseArray<vtkStdString>> bb1 =
      vtkSmartPointer<vtkDenseArray<vtkStdString>>::New();
    bb1->SetName("bb1");
    bb1->Resize(3);
    bb1->SetValue(0, "The");
    bb1->SetValue(1, "quick brown");
    bb1->SetValue(2, "fox");

    std::stringstream bb_buffer;
    vtkArrayWriter::Write(bb1, bb_buffer, true);
    vtkSmartPointer<vtkArray> bb2;
    bb2.TakeReference(vtkArrayReader::Read(bb_buffer));

    test_expression(bb2);
    test_expression(bb2->GetName() == "bb1");
    test_expression(vtkDenseArray<vtkStdString>::SafeDownCast(bb2));
    test_expression(bb2->GetNonNullSize() == 3);
    test_expression(bb2->GetVariantValue(0).ToString() == "The");
    test_expression(bb2->GetVariantValue(1).ToString() == "quick brown");
    test_expression(bb2->GetVariantValue(2).ToString() == "fox");

    // Test sparse string arrays containing whitespace ...
    vtkSmartPointer<vtkSparseArray<vtkStdString>> bc1 =
      vtkSmartPointer<vtkSparseArray<vtkStdString>>::New();
    bc1->Resize(3);
    bc1->SetNullValue("empty space");
    bc1->SetValue(0, "The");
    bc1->SetValue(1, "quick brown");
    bc1->SetValue(2, "fox");

    std::stringstream bc_buffer;
    vtkArrayWriter::Write(bc1, bc_buffer, true);
    vtkSmartPointer<vtkArray> bc2;
    bc2.TakeReference(vtkArrayReader::Read(bc_buffer));

    test_expression(bc2);
    test_expression(vtkSparseArray<vtkStdString>::SafeDownCast(bc2));
    test_expression(bc2->GetNonNullSize() == 3);
    test_expression(
      vtkSparseArray<vtkStdString>::SafeDownCast(bc2)->GetNullValue() == "empty space");
    test_expression(bc2->GetVariantValue(0).ToString() == "The");
    test_expression(bc2->GetVariantValue(1).ToString() == "quick brown");
    test_expression(bc2->GetVariantValue(2).ToString() == "fox");

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
