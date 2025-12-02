// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "vtkIOODBCTestingCxxConfigure.h"
#include "vtkODBCDatabase.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLQuery.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <sstream>

#include <iostream>

#define LONGSTRING "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"

int TestODBCDatabase(int, char** const)
{
  vtkODBCDatabase* db = vtkODBCDatabase::New();
  db->SetDataSourceName(VTK_ODBC_TEST_DSN);
  bool status = db->Open(nullptr);

  if (!status)
  {
    std::cerr << "Couldn't open database.  Error message: " << db->GetLastErrorText() << "\n";
    return 1;
  }

  vtkSQLQuery* query = db->GetQueryInstance();

  std::string createQuery("CREATE TABLE people (name VARCHAR(1024), age INTEGER, weight FLOAT)");
  std::cout << createQuery << std::endl;
  query->SetQuery(createQuery.c_str());
  if (!query->Execute())
  {
    std::cerr << "Create query failed.  Error message: " << query->GetLastErrorText() << std::endl;
    return 1;
  }

  int i;
  for (i = 0; i < 20; ++i)
  {
    std::ostringstream queryBuf;

    queryBuf << "INSERT INTO people VALUES('John Doe " << i << "', " << i << ", " << 10 * i + 0.5
             << ")";
    std::cout << queryBuf.str() << std::endl;
    query->SetQuery(queryBuf.str().c_str());
    if (!query->Execute())
    {
      std::cerr << "Insert query " << i << " failed.  Error message: " << query->GetLastErrorText()
                << std::endl;
      return 1;
    }
  }

  const char* placeholders = "INSERT INTO people (name, age, weight) VALUES (?, ?, ?)";
  query->SetQuery(placeholders);
  for (i = 21; i < 40; i++)
  {
    auto name = vtk::format("John Doe {:d}", i);
    bool bind1 = query->BindParameter(0, name);
    bool bind2 = query->BindParameter(1, i);
    bool bind3 = query->BindParameter(2, 10.1 * i);
    if (!(bind1 && bind2 && bind3))
    {
      std::cerr << "Parameter binding failed on query " << i << ": " << bind1 << " " << bind2 << " "
                << bind3 << std::endl;
      return 1;
    }
    std::cout << query->GetQuery() << std::endl;
    if (!query->Execute())
    {
      std::cerr << "Insert query " << i << " failed" << std::endl;
      return 1;
    }
  }

  const char* queryText = "SELECT name, age, weight FROM people WHERE age <= 30";
  query->SetQuery(queryText);
  std::cerr << std::endl << "Running query: " << query->GetQuery() << std::endl;

  std::cerr << std::endl << "Using vtkSQLQuery directly to execute query:" << std::endl;
  if (!query->Execute())
  {
    std::cerr << "Query failed with error message " << query->GetLastErrorText() << std::endl;
    return 1;
  }

  std::cerr << "Fields returned by query: ";
  for (int col = 0; col < query->GetNumberOfFields(); ++col)
  {
    if (col > 0)
    {
      std::cerr << ", ";
    }
    std::cerr << query->GetFieldName(col);
  }
  std::cerr << std::endl;
  int thisRow = 0;
  while (query->NextRow())
  {
    std::cerr << "Row " << thisRow << ": ";
    ++thisRow;
    for (int field = 0; field < query->GetNumberOfFields(); ++field)
    {
      if (field > 0)
      {
        std::cerr << ", ";
      }
      std::cerr << query->DataValue(field).ToString();
    }
    std::cerr << std::endl;
  }

  std::cerr << std::endl << "Using vtkSQLQuery to execute query and retrieve by row:" << std::endl;
  if (!query->Execute())
  {
    std::cerr << "Query failed with error message " << query->GetLastErrorText() << std::endl;
    return 1;
  }
  for (int col = 0; col < query->GetNumberOfFields(); ++col)
  {
    if (col > 0)
    {
      std::cerr << ", ";
    }
    std::cerr << query->GetFieldName(col);
  }
  std::cerr << std::endl;
  vtkVariantArray* va = vtkVariantArray::New();
  while (query->NextRow(va))
  {
    for (int field = 0; field < va->GetNumberOfValues(); ++field)
    {
      if (field > 0)
      {
        std::cerr << ", ";
      }
      std::cerr << va->GetValue(field).ToString();
    }
    std::cerr << std::endl;
  }
  va->Delete();

  std::cerr << std::endl << "Using vtkRowQueryToTable to execute query:" << std::endl;
  vtkRowQueryToTable* reader = vtkRowQueryToTable::New();
  reader->SetQuery(query);
  reader->Update();
  vtkTable* table = reader->GetOutput();
  for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
  {
    table->GetColumn(col)->Print(std::cerr);
  }
  std::cerr << std::endl;

#if defined(PRINT_TABLE_CONTENTS)
  for (vtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
  {
    for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
    {
      vtkVariant v = table->GetValue(row, col);
      std::cerr << "row " << row << ", col " << col << " - " << v.ToString() << " ( "
                << vtkImageScalarTypeNameMacro(v.GetType()) << " )" << std::endl;
    }
  }
#endif

  query->SetQuery("DROP TABLE people");
  query->Execute();

  reader->Delete();
  query->Delete();
  db->Delete();

  return 0;
}
