// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "DatabaseSchemaWith2Tables.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkSQLQuery.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkTestErrorObserver.h"

#include <iostream>

int TestSQLiteDatabase(int /*argc*/, char* /*argv*/[])
{
  bool status;

  std::cerr << ">>>>> Testing bad input." << std::endl;

  vtkSQLDatabase* db0 = vtkSQLDatabase::CreateFromURL(nullptr);
  if (db0)
  {
    std::cerr << "ERROR: Created a database from a nullptr URL! How?" << std::endl;
    db0->Delete();
    return 1;
  }

  std::cerr << ">>>>> Testing creation modes." << std::endl;

  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkTest::ErrorObserver> queryObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSQLiteDatabase* db1 =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://local.db"));

  status = db1->Open("", vtkSQLiteDatabase::CREATE_OR_CLEAR);
  vtkSQLQuery* query1 = db1->GetQueryInstance();
  query1->SetQuery("CREATE TABLE test (id INTEGER)");
  if (!query1->Execute())
  {
    std::cerr << "Create query failed" << std::endl;
    return 1;
  }
  if (!status)
  {
    std::cerr << "Couldn't open database using CREATE_OR_CLEAR.\n";
    return 1;
  }
  db1->Delete();
  query1->Delete();

  vtkSQLiteDatabase* db2 =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://local.db"));
  db2->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  status = db2->Open("", vtkSQLiteDatabase::CREATE);
  if (status)
  {
    std::cerr << "Using CREATE on an existing file should have failed but did not.\n";
    return 1;
  }
  int status1 =
    errorObserver->CheckErrorMessage("You specified creating a database but the file exists");
  if (status1 != 0)
  {
    std::cerr << "Expected error message not found.\n";
    return 1;
  }
  db2->Delete();

  vtkSQLiteDatabase* db3 =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://local.db"));
  status = db3->Open("", vtkSQLiteDatabase::USE_EXISTING_OR_CREATE);
  if (!status)
  {
    std::cerr << "Using USE_EXISTING_OR_CREATE did not work.\n";
    return 1;
  }
  vtkSQLQuery* query3 = db3->GetQueryInstance();
  query3->SetQuery("SELECT * from test");
  if (!query3->Execute())
  {
    std::cerr << "Select query failed" << std::endl;
    return 1;
  }
  db3->Delete();
  query3->Delete();

  vtkSQLiteDatabase* db4 =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://local.db"));
  status = db4->Open("", vtkSQLiteDatabase::CREATE_OR_CLEAR);
  if (!status)
  {
    std::cerr << "Using CREATE_OR_CLEAR did not work.\n";
    return 1;
  }
  vtkSQLQuery* query4 = db4->GetQueryInstance();
  query4->AddObserver(vtkCommand::ErrorEvent, queryObserver);
  query4->SetQuery("SELECT * from test");
  if (query4->Execute())
  {
    std::cerr << "Select query succeeded when it shouldn't have." << std::endl;
    return 1;
  }
  status1 = queryObserver->CheckErrorMessage("Query is not null but prepared statement is");
  db4->Delete();
  query4->Delete();
  if (status1 != 0)
  {
    std::cerr << "Expected error message not found.\n";
    return 1;
  }

  std::cerr << ">>>>> Testing database functions" << std::endl;

  vtkSQLiteDatabase* db =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://:memory:"));
  status = db->Open("");

  if (!status)
  {
    std::cerr << "Couldn't open database.\n";
    return 1;
  }

  vtkSQLQuery* query = db->GetQueryInstance();

  std::string createQuery(
    "CREATE TABLE IF NOT EXISTS people (name TEXT, age INTEGER, weight FLOAT)");
  std::cout << createQuery << std::endl;
  query->SetQuery(createQuery.c_str());
  if (!query->Execute())
  {
    std::cerr << "Create query failed" << std::endl;
    return 1;
  }

  int i;
  for (i = 0; i < 20; i++)
  {
    auto insertQuery = vtk::format(
      "INSERT INTO people (name, age, weight) VALUES('John Doe {:d}', {:d}, {:f})", i, i, 10.1 * i);
    std::cout << insertQuery << std::endl;
    query->SetQuery(insertQuery.c_str());
    if (!query->Execute())
    {
      std::cerr << "Insert query " << i << " failed" << std::endl;
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

  const char* queryText = "SELECT name, age, weight FROM people WHERE age <= 20";
  query->SetQuery(queryText);
  std::cerr << std::endl << "Running query: " << query->GetQuery() << std::endl;

  std::cerr << std::endl << "Using vtkSQLQuery directly to execute query:" << std::endl;
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    return 1;
  }

  for (int col = 0; col < query->GetNumberOfFields(); col++)
  {
    if (col > 0)
    {
      std::cerr << ", ";
    }
    std::cerr << query->GetFieldName(col);
  }
  std::cerr << std::endl;
  while (query->NextRow())
  {
    for (int field = 0; field < query->GetNumberOfFields(); field++)
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
    std::cerr << "Query failed" << std::endl;
    return 1;
  }
  for (int col = 0; col < query->GetNumberOfFields(); col++)
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
    for (int field = 0; field < va->GetNumberOfValues(); field++)
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
  for (vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
  {
    table->GetColumn(col)->Print(std::cerr);
  }
  std::cerr << std::endl;
  for (vtkIdType row = 0; row < table->GetNumberOfRows(); row++)
  {
    for (vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
    {
      vtkVariant v = table->GetValue(row, col);
      std::cerr << "row " << row << ", col " << col << " - " << v.ToString() << " ("
                << vtkImageScalarTypeNameMacro(v.GetType()) << ")" << std::endl;
    }
  }

  reader->Delete();
  query->Delete();
  db->Delete();

  // ----------------------------------------------------------------------
  // Testing transformation of a schema into a SQLite database

  // 1. Create the schema
  DatabaseSchemaWith2Tables schema;

  // 2. Convert the schema into a SQLite database
  std::cerr << "@@ Converting the schema into a SQLite database...";

  vtkSQLiteDatabase* dbSch =
    vtkSQLiteDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL("sqlite://:memory:"));
  status = dbSch->Open("");

  if (!status)
  {
    std::cerr << "Couldn't open database.\n";
    return 1;
  }

  status = dbSch->EffectSchema(schema.GetSchema());
  if (!status)
  {
    std::cerr << "Could not effect test schema.\n";
    return 1;
  }
  std::cerr << " done." << std::endl;

  // 3. Count tables of the newly created database
  std::cerr << "@@ Fetching table names of the newly created database:\n";

  query = dbSch->GetQueryInstance();

  query->SetQuery("SELECT name FROM sqlite_master WHERE type = \"table\"");
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    return 1;
  }

  std::vector<std::string> tables;
  int tblHandle = 0;
  for (; query->NextRow(); ++tblHandle)
  {
    std::string tblNameSch(schema->GetTableNameFromHandle(tblHandle));
    std::string tblNameDB(query->DataValue(0).ToString());
    std::cerr << "     " << tblNameDB << "\n";

    if (tblNameDB != tblNameSch)
    {
      std::cerr << "Fetched an incorrect name: " << tblNameDB << " != " << tblNameSch << std::endl;
      return 1;
    }

    tables.push_back(tblNameDB);
  }

  if (tblHandle != schema->GetNumberOfTables())
  {
    std::cerr << "Found an incorrect number of tables: " << tblHandle
              << " != " << schema->GetNumberOfTables() << std::endl;
    return 1;
  }

  std::cerr << "   " << tblHandle << " found.\n";

  // 4. Test EscapeString.
  std::cerr << "@@ Escaping a naughty string...";

  std::string queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( " +
    query->EscapeString(vtkStdString("Str\"ang'eS\ntring"), true) + ", 2 )";
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << " done." << std::endl;

  // 5. Read back the escaped string to verify it worked.
  std::cerr << "@@ Reading it back... <";

  queryStr = "SELECT somename FROM atable WHERE somenmbr=2";
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  if (!query->NextRow())
  {
    std::cerr << "Query returned no results" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << query->DataValue(0).ToString() << "> ";
  std::cerr << " done." << std::endl;

  // 6. Drop tables
  std::cerr << "@@ Dropping these tables...";

  for (std::vector<std::string>::iterator it = tables.begin(); it != tables.end(); ++it)
  {
    queryStr = "DROP TABLE ";
    queryStr += *it;
    query->SetQuery(queryStr.c_str());

    if (!query->Execute())
    {
      std::cerr << "Query failed" << std::endl;
      return 1;
    }
  }

  std::cerr << " done." << std::endl;

  // Clean up
  dbSch->Delete();
  query->Delete();

  return 0;
}
