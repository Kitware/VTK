/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMergeGraphs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkDelimitedTextReader.h>
#include <vtkMergeGraphs.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTableToGraph.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkTestUtilities.h>
#include <vtkIOStream.h>

#include <string>

template <typename T, size_t Cols>
void BuildTable(vtkTable* table, T values[][Cols], int rows)
{
  for (size_t c = 0; c < Cols; ++c)
    {
    vtkSmartPointer<vtkStringArray> arr = vtkSmartPointer<vtkStringArray>::New();
    arr->SetName(vtkVariant(values[0][c]).ToString());
    for (int r = 0; r < rows; ++r)
      {
      arr->InsertNextValue(values[r+1][c]);
      }
    table->AddColumn(arr);
    }
}

bool CheckTable(vtkTable* expected, vtkTable* output)
{
  bool ok = true;
  for (vtkIdType col = 0; col < expected->GetNumberOfColumns(); ++col)
    {
    vtkStringArray* exp_arr = vtkStringArray::SafeDownCast(expected->GetColumn(col));
    vtkStringArray* out_arr = vtkStringArray::SafeDownCast(output->GetColumnByName(exp_arr->GetName()));
    if (!out_arr)
      {
      cerr << "Output array " << exp_arr->GetName() << " does not exist" << endl;
      ok = false;
      continue;
      }
    if (out_arr->GetNumberOfTuples() != exp_arr->GetNumberOfTuples())
      {
      cerr << "Output array " << exp_arr->GetName() << " has " << out_arr->GetNumberOfTuples() << " tuples when " << exp_arr->GetNumberOfTuples() << " were expected." << endl;
      ok = false;
      continue;
      }
    for (vtkIdType row = 0; row < exp_arr->GetNumberOfTuples(); ++row)
      {
      if (exp_arr->GetValue(row) != out_arr->GetValue(row))
        {
        cerr << "Output array " << exp_arr->GetName() << " has " << out_arr->GetValue(row) << " at position " << row << " when " << exp_arr->GetValue(row) << " was expected." << endl;
        ok = false;
        }
      }
    }
  return ok;
}

std::string vert_data1[][3] = {
  { "id", "arr1", "arr2" },
  { "v1", "a"   , "d" },
  { "v2", "b"   , "e" },
  { "v3", "c"   , "f" }
};

std::string vert_data2[][3] = {
  { "id", "arr2", "arr3" },
  { "v2", "g"   , "j" },
  { "v3", "h"   , "k" },
  { "v4", "i"   , "l" }
};

std::string edge_data1[][4] = {
  { "id", "src", "tgt", "extra" },
  { "e1", "v1" , "v2" , "m" },
  { "e2", "v2" , "v3" , "n" },
  { "e3", "v3" , "v1" , "o" }
};

std::string edge_data2[][3] = {
  { "id", "src", "tgt" },
  { "e4", "v2" , "v3" },
  { "e5", "v3" , "v4" },
  { "e6", "v4" , "v2" }
};

std::string expected_vert_data[][3] = {
  { "id", "arr1", "arr2" },
  { "v1", "a", "d" },
  { "v2", "b", "e" },
  { "v3", "c", "f" },
  { "v4", "" , "i" }
};

std::string expected_edge_data[][4] = {
  { "id", "src", "tgt", "extra" },
  { "e1", "v1" , "v2" , "m" },
  { "e2", "v2" , "v3" , "n" },
  { "e3", "v3" , "v1" , "o" },
  { "e4", "v2" , "v3" , "" },
  { "e6", "v4" , "v2" , "" },
  { "e5", "v3" , "v4" , "" }
};

int TestMergeGraphs(int, char*[])
{
  vtkSmartPointer<vtkTable> vert_table1 = vtkSmartPointer<vtkTable>::New();
  BuildTable(vert_table1, vert_data1, 3);
  vtkSmartPointer<vtkTable> vert_table2 = vtkSmartPointer<vtkTable>::New();
  BuildTable(vert_table2, vert_data2, 3);
  vtkSmartPointer<vtkTable> edge_table1 = vtkSmartPointer<vtkTable>::New();
  BuildTable(edge_table1, edge_data1, 3);
  vtkSmartPointer<vtkTable> edge_table2 = vtkSmartPointer<vtkTable>::New();
  BuildTable(edge_table2, edge_data2, 3);

  vtkSmartPointer<vtkTableToGraph> ttg1 = vtkSmartPointer<vtkTableToGraph>::New();
  ttg1->SetInputData(0, edge_table1);
  ttg1->SetInputData(1, vert_table1);
  ttg1->AddLinkVertex("src", "id");
  ttg1->AddLinkVertex("tgt", "id");
  ttg1->AddLinkEdge("src", "tgt");
  vtkSmartPointer<vtkTableToGraph> ttg2 = vtkSmartPointer<vtkTableToGraph>::New();
  ttg2->SetInputData(0, edge_table2);
  ttg2->SetInputData(1, vert_table2);
  ttg2->AddLinkVertex("src", "id");
  ttg2->AddLinkVertex("tgt", "id");
  ttg2->AddLinkEdge("src", "tgt");

  vtkSmartPointer<vtkMergeGraphs> merge = vtkSmartPointer<vtkMergeGraphs>::New();
  merge->SetInputConnection(0, ttg1->GetOutputPort());
  merge->SetInputConnection(1, ttg2->GetOutputPort());

  merge->Update();

  vtkSmartPointer<vtkTable> output_vert_table = vtkSmartPointer<vtkTable>::New();
  output_vert_table->SetRowData(merge->GetOutput()->GetVertexData());
  output_vert_table->Dump(10);

  vtkSmartPointer<vtkTable> output_edge_table = vtkSmartPointer<vtkTable>::New();
  output_edge_table->SetRowData(merge->GetOutput()->GetEdgeData());
  output_edge_table->Dump(10);

  // Check the results
  int ret = 0;
  vtkSmartPointer<vtkTable> expected_vert_table = vtkSmartPointer<vtkTable>::New();
  BuildTable(expected_vert_table, expected_vert_data, 4);
  if (!CheckTable(expected_vert_table, output_vert_table))
    {
    ret = 1;
    }
  vtkSmartPointer<vtkTable> expected_edge_table = vtkSmartPointer<vtkTable>::New();
  BuildTable(expected_edge_table, expected_edge_data, 6);
  if (!CheckTable(expected_edge_table, output_edge_table))
    {
    ret = 1;
    }

  return ret;
}
