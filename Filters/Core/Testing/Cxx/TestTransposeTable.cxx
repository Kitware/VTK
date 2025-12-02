// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTransposeTable.h"

#include <sstream>

#include <iostream>

int TestTransposeTable(int, char*[])
{
  vtkNew<vtkTable> table;

  constexpr int nbValues = 5;

  vtkNew<vtkStringArray> colName;
  colName->SetName("Name");
  colName->SetNumberOfValues(nbValues);
  colName->SetValue(0, "apple");
  colName->SetValue(1, "milk");
  colName->SetValue(2, "cream");
  colName->SetValue(3, "pasta");
  colName->SetValue(4, "tomato");
  table->AddColumn(colName);

  vtkNew<vtkIntArray> colId;
  colId->SetName("Id");
  colId->SetNumberOfValues(nbValues);
  colId->SetValue(0, 0);
  colId->SetValue(1, 1);
  colId->SetValue(2, 2);
  colId->SetValue(3, 3);
  colId->SetValue(4, 4);
  table->AddColumn(colId);

  vtkNew<vtkDoubleArray> colDouble;
  colDouble->SetName("Double");
  colDouble->SetNumberOfValues(nbValues);
  colDouble->SetValue(0, 5.);
  colDouble->SetValue(1, 4.005);
  colDouble->SetValue(2, 2.65);
  colDouble->SetValue(3, 1.1);
  colDouble->SetValue(4, 0.4);
  table->AddColumn(colDouble);

  vtkNew<vtkFloatArray> colFloat;
  colFloat->SetName("Float");
  colFloat->SetNumberOfValues(nbValues);
  colFloat->SetValue(0, 15.f);
  colFloat->SetValue(1, 14.005f);
  colFloat->SetValue(2, 12.65f);
  colFloat->SetValue(3, 11.1f);
  colFloat->SetValue(4, 10.4f);
  table->AddColumn(colFloat);

  // Transpose the input table
  vtkNew<vtkTransposeTable> filter;
  filter->SetInputData(table);
  filter->Update();

  vtkTable* outTable = filter->GetOutput();

  if (table->GetNumberOfColumns() != outTable->GetNumberOfRows())
  {
    std::cout << "Input table:" << std::endl;
    table->Dump();
    std::cout << "Transposed table:" << std::endl;
    outTable->Dump();
    std::cout << "Failed: Column/row mismatched!" << std::endl;
    return EXIT_FAILURE;
  }

  if (table->GetNumberOfRows() != outTable->GetNumberOfColumns() - 1)
  {
    std::cout << "Input table:" << std::endl;
    table->Dump();
    std::cout << "Transposed table:" << std::endl;
    outTable->Dump();
    std::cout << "Failed: Row/Column mismatched!" << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < table->GetNumberOfRows(); i++)
  {
    std::stringstream ss;
    ss << i;
    vtkAbstractArray* col = outTable->GetColumnByName(ss.str().c_str());
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
    {
      if (col->GetVariantValue(j) != table->GetValue(i, j))
      {
        std::cout << "Failed: Column/row mismatched!" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  // Let's transpose the transposed table and compare it with input table
  vtkNew<vtkTransposeTable> filter2;
  filter2->SetInputData(outTable);
  filter2->SetAddIdColumn(false);
  filter2->SetUseIdColumn(true);
  filter2->Update();

  vtkTable* outTable2 = filter2->GetOutput();

  for (int i = 0; i < table->GetNumberOfRows(); i++)
  {
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
    {
      if (table->GetValue(i, j) != outTable2->GetValue(i, j))
      {
        std::cout << "Transposed of transposed table:" << std::endl;
        outTable2->Dump();
        std::cout << "Failed: Column/row mismatch!" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
