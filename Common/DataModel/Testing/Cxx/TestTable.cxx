/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTable.cxx

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
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <ctime>
#include <vector>

const int size = 100;
const double prob = 1.0 - 1.0 / size;
const double highProb = 1.0 - 1.0 / (size * size);

void CheckEqual(
  vtkTable* table, std::vector<std::vector<double>>& stdTable, const std::string& info)
{
  // Check sizes
  if (table->GetNumberOfRows() != static_cast<vtkIdType>(stdTable[0].size()))
  {
    cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
    cout << "Number of rows is incorrect (" << table->GetNumberOfRows() << " != " << stdTable.size()
         << ")" << endl;
    exit(EXIT_FAILURE);
  }
  if (table->GetNumberOfColumns() != static_cast<vtkIdType>(stdTable.size()))
  {
    cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
    cout << "Number of columns is incorrect (" << table->GetNumberOfColumns()
         << " != " << stdTable.size() << ")" << endl;
    exit(EXIT_FAILURE);
  }

  // Use GetValue() and GetValueByName() to check
  for (int i = 0; i < table->GetNumberOfRows(); i++)
  {
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
    {
      double tableVal = table->GetValue(i, j).ToDouble();
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && tableVal != stdTableVal)
      {
        cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << tableVal << " != " << stdTableVal << ")" << endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  // Use GetColumn() and GetColumnByName() to check
  for (int j = 0; j < table->GetNumberOfColumns(); j++)
  {
    vtkAbstractArray* arr;
    if (vtkMath::Random() < 0.5)
    {
      arr = table->GetColumn(j);
    }
    else
    {
      arr = table->GetColumnByName(table->GetColumnName(j));
    }
    for (int i = 0; i < table->GetNumberOfRows(); i++)
    {
      double val;
      if (arr->IsA("vtkVariantArray"))
      {
        val = vtkArrayDownCast<vtkVariantArray>(arr)->GetValue(i).ToDouble();
      }
      else if (arr->IsA("vtkStringArray"))
      {
        vtkVariant v(vtkArrayDownCast<vtkStringArray>(arr)->GetValue(i));
        val = v.ToDouble();
      }
      else if (arr->IsA("vtkDataArray"))
      {
        val = vtkArrayDownCast<vtkDataArray>(arr)->GetTuple1(i);
      }
      else
      {
        cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
        cout << "Unknown array type" << endl;
        exit(EXIT_FAILURE);
      }
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && val != stdTableVal)
      {
        cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << val << " != " << stdTableVal << ")" << endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  // Use GetRow() to check
  for (int i = 0; i < table->GetNumberOfRows(); i++)
  {
    vtkVariantArray* arr = table->GetRow(i);
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
    {
      double val = arr->GetValue(j).ToDouble();
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && val != stdTableVal)
      {
        cout << "TestTable CheckEqual() fails after operation '" << info << "()':" << endl;
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << val << " != " << stdTableVal << ")" << endl;
        exit(EXIT_FAILURE);
      }
    }
  }
}

void FillTable(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Creating columns." << endl;
  vtkIdType columnId = 0;
  bool noColumns = true;
  while (noColumns || vtkMath::Random() < prob)
  {
    noColumns = false;

    stdTable.emplace_back();

    double r = vtkMath::Random();
    vtkVariant name(columnId);
    vtkAbstractArray* arr;
    if (r < 0.25)
    {
      arr = vtkIntArray::New();
      arr->SetName((name.ToString() + " (vtkIntArray)").c_str());
    }
    else if (r < 0.5)
    {
      arr = vtkDoubleArray::New();
      arr->SetName((name.ToString() + " (vtkDoubleArray)").c_str());
    }
    else if (r < 0.75)
    {
      arr = vtkStringArray::New();
      arr->SetName((name.ToString() + " (vtkStringArray)").c_str());
    }
    else
    {
      arr = vtkVariantArray::New();
      arr->SetName((name.ToString() + " (vtkVariantArray)").c_str());
    }
    table->AddColumn(arr);
    arr->Delete();
    columnId++;
  }

  CheckEqual(table, stdTable, "FillTable");
}

void AddColumn(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  vtkVariant name(table->GetNumberOfColumns());
  vtkNew<vtkDoubleArray> arr;
  arr->SetNumberOfComponents(1);
  arr->SetNumberOfTuples(table->GetNumberOfRows());
  arr->SetName((name.ToString() + " (vtkDoubleArray)").c_str());
  arr->FillComponent(0, 0.0);
  table->AddColumn(arr);

  std::vector<double> fillvec;
  fillvec.resize(table->GetNumberOfRows(), 0.0);
  stdTable.push_back(fillvec);

  CheckEqual(table, stdTable, "AddColumn");
}

void InsertColumn(vtkTable* table, std::vector<std::vector<double>>& stdTable, int c0)
{
  vtkVariant name(table->GetNumberOfColumns());
  vtkNew<vtkDoubleArray> arr;
  arr->SetNumberOfComponents(1);
  arr->SetNumberOfTuples(table->GetNumberOfRows());
  arr->SetName((name.ToString() + " (vtkDoubleArray)").c_str());
  arr->FillComponent(0, 0.0);
  table->InsertColumn(arr, c0);

  std::vector<double> fillvec;
  fillvec.resize(table->GetNumberOfRows(), 0.0);
  stdTable.insert(stdTable.begin() + c0, fillvec);

  CheckEqual(table, stdTable, "InsertColumn");
}

void InsertRows(
  vtkTable* table, std::vector<std::vector<double>>& stdTable, const vtkIdType r0, const int n)
{
  cout << "Inserting rows in middle of table." << endl;
  const int ncols = table->GetNumberOfColumns();

  // insert rows in middle of stdTable, fill with 0.0
  for (int c = 0; c < ncols; c++)
  {
    for (int r = r0; r < r0 + n; r++)
    {
      stdTable[c].insert(stdTable[c].begin() + r0, 0.0);
    }
  }

  // insert rows in middle of vtkTable
  // then fill with defined value, i.e. 0.0
  table->InsertRows(r0, n);
  for (int r = r0; r < r0 + n; r++)
  {
    for (int c = 0; c < ncols; c++)
    {
      table->SetValue(r, c, 0.0);
    }
  }

  // compare if the remainder of the tables still equal each other
  CheckEqual(table, stdTable, "InsertRowsInMiddle");
}

void InsertEmptyRows(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Inserting empty rows." << endl;
  bool noRows = true;
  while (noRows || vtkMath::Random() < prob)
  {
    noRows = false;
    table->InsertNextBlankRow();
    for (unsigned int i = 0; i < stdTable.size(); i++)
    {
      stdTable[i].push_back(0.0);
    }
  }
  CheckEqual(table, stdTable, "InsertEmptyRows");
}

void InsertFullRows(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Inserting full rows." << endl;
  while (vtkMath::Random() < prob)
  {
    vtkVariantArray* rowArray = vtkVariantArray::New();
    for (vtkIdType j = 0; j < table->GetNumberOfColumns(); j++)
    {
      rowArray->InsertNextValue(vtkVariant(j));
      stdTable[j].push_back(j);
    }
    table->InsertNextRow(rowArray);
    rowArray->Delete();
  }
  CheckEqual(table, stdTable, "InsertFullRows");
}

void RandomizeValues(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Performing all kinds of inserts." << endl;
  int id = 0;
  while (vtkMath::Random() < highProb)
  {
    vtkIdType row = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfRows()));
    vtkIdType col = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfColumns()));
    vtkVariant v;
    if (vtkMath::Random() < 0.25)
    {
      vtkVariant temp(id);
      v = vtkVariant(temp.ToString());
    }
    else if (vtkMath::Random() < 0.5)
    {
      v = vtkVariant(id);
    }
    else
    {
      v = vtkVariant(static_cast<double>(id));
    }

    if (vtkMath::Random() < 0.5)
    {
      table->SetValue(row, col, v);
    }
    else
    {
      table->SetValueByName(row, table->GetColumnName(col), v);
    }
    stdTable[col][row] = id;

    id++;
  }
  CheckEqual(table, stdTable, "RandomInserts");
}

void RemoveHalfOfRows(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Removing half of the rows." << endl;
  int numRowsToRemove = table->GetNumberOfRows() / 2;
  for (int i = 0; i < numRowsToRemove; i++)
  {
    vtkIdType row = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfRows()));
    cout << "Removing row " << row << " from vtkTable with " << table->GetNumberOfRows() << " rows"
         << endl;
    table->RemoveRow(row);

    cout << "Removing row " << row << " from std::vector< std::vector<double> > with "
         << stdTable[0].size() << " rows " << endl;
    for (unsigned int j = 0; j < stdTable.size(); j++)
    {
      std::vector<double>::iterator rowIt = stdTable[j].begin() + row;
      stdTable[j].erase(rowIt);
    }
  }
  CheckEqual(table, stdTable, "RemoveHalfRows");
}

void SqueezeRows(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  table->SqueezeRows();
  CheckEqual(table, stdTable, "SqueezeRows");
}

void RemoveHalfOfColumns(vtkTable* table, std::vector<std::vector<double>>& stdTable)
{
  cout << "Removing half of the columns." << endl;
  int numColsToRemove = table->GetNumberOfColumns() / 2;
  for (int i = 0; i < numColsToRemove; i++)
  {
    vtkIdType col = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfColumns()));
    if (vtkMath::Random() < 0.5)
    {
      table->RemoveColumn(col);
    }
    else
    {
      table->RemoveColumnByName(table->GetColumnName(col));
    }

    std::vector<std::vector<double>>::iterator colIt = stdTable.begin() + col;
    stdTable.erase(colIt);
  }
  CheckEqual(table, stdTable, "RemoveHalfColumns");
}

int TestTable(int, char*[])
{
  cout << "CTEST_FULL_OUTPUT" << endl;

  long seed = time(nullptr);
  cout << "Seed: " << seed << endl;
  vtkMath::RandomSeed(seed);

  // Make a table and a parallel vector of vectors containing the same data
  vtkNew<vtkTable> table;
  std::vector<std::vector<double>> stdTable;
  FillTable(table, stdTable);

  InsertEmptyRows(table, stdTable);
  RandomizeValues(table, stdTable);

  InsertFullRows(table, stdTable);
  RandomizeValues(table, stdTable);

  // add new column to the end of the table
  AddColumn(table, stdTable);
  RandomizeValues(table, stdTable);

  // insert new column in the middle of the table
  InsertColumn(table, stdTable, table->GetNumberOfColumns() / 2);
  RandomizeValues(table, stdTable);

  // insert new rows at the beginning of the table
  InsertRows(table, stdTable, 0, 3);
  RandomizeValues(table, stdTable);

  // insert new rows in the middle of the table
  InsertRows(table, stdTable, table->GetNumberOfRows() / 2, 3);
  RandomizeValues(table, stdTable);

  // insert new rows at the end of the table
  InsertRows(table, stdTable, table->GetNumberOfRows() - 1, 3);
  RandomizeValues(table, stdTable);

  RemoveHalfOfRows(table, stdTable);
  RandomizeValues(table, stdTable);

  SqueezeRows(table, stdTable);
  RandomizeValues(table, stdTable);

  RemoveHalfOfColumns(table, stdTable);
  RandomizeValues(table, stdTable);

  return EXIT_SUCCESS;
}
