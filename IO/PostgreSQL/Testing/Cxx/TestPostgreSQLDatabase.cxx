// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "DatabaseSchemaWith2Tables.h"
#include "vtkIOPostgresSQLTestingCxxConfigure.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkSQLQuery.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include <vector>

#include <iostream>

int TestPostgreSQLDatabase(int /*argc*/, char* /*argv*/[])
{
  // This test requires the user in VTK_PSQL_TEST_URL to have
  // permission to create and drop the database named in that URL
  // as well as tables in that database. That user must also be
  // able to connect to the "template1" database (which initdb
  // creates and should be present on all systems -- we do NOT
  // support non-standard configurations where this is not true).
  vtkPostgreSQLDatabase* db =
    vtkPostgreSQLDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL(VTK_PSQL_TEST_URL));
  std::string realDatabase = db->GetDatabaseName();
  db->SetDatabaseName("template1"); // This is guaranteed to exist
  bool status = db->Open();
  if (!status)
  {
    std::cerr << "Couldn't open database.\nError message: \"" << db->GetLastErrorText() << "\"\n";
    db->Delete();
    return 1;
  }

  vtkStringArray* dbNames = db->GetDatabases();
  std::cout << "Database list:\n";
  for (int dbi = 0; dbi < dbNames->GetNumberOfValues(); ++dbi)
  {
    std::cout << "+ " << dbNames->GetValue(dbi) << std::endl;
  }
  dbNames->Delete();
  if (!db->CreateDatabase(realDatabase.c_str(), true))
  {
    std::cerr << "Error: " << db->GetLastErrorText() << std::endl;
  }

  vtkSQLQuery* query = db->GetQueryInstance();

  // Force a database connection close
  // This also forces us to connect to the database named in the test URL.
  std::string fauxDatabase = realDatabase + "blarney";
  db->SetDatabaseName(fauxDatabase.c_str());
  db->SetDatabaseName(realDatabase.c_str());

  if (!db->Open())
  {
    std::cerr << "Error: " << db->GetLastErrorText() << std::endl;
  }

  // Test that bad queries fail without segfaulting...
  std::string dropQuery("DROP TABLE people");
  std::cout << dropQuery << std::endl;
  query->SetQuery(dropQuery.c_str());
  if (!query->Execute())
  {
    std::cout << "Drop query did not succeed (this result *** was *** expected). The last message: "
              << std::endl;
    std::cout << "   " << query->GetLastErrorText() << std::endl;
  }
  else
  {
    std::cerr << "The query \"DROP TABLE people\" succeeded when it should not have.\n";
  }

  // Test table creation, insertion, queries
  std::string createQuery("CREATE TABLE people (name TEXT, age INTEGER, weight FLOAT)");
  std::cout << createQuery << std::endl;
  query->SetQuery(createQuery.c_str());
  if (!query->Execute())
  {
    std::cerr << "Create query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  for (int i = 0; i < 40; ++i)
  {
    auto insertQuery =
      vtk::format("INSERT INTO people VALUES('John Manyjars {:d}', {:d}, {:d})", i, i, 10 * i);
    std::cout << insertQuery << std::endl;
    query->SetQuery(insertQuery);
    if (!query->Execute())
    {
      std::cerr << "Insert query " << i << " failed" << std::endl;
      query->Delete();
      db->Delete();
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
    query->Delete();
    db->Delete();
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
  while (query->NextRow())
  {
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
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
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
  for (vtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
  {
    for (vtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
    {
      vtkVariant v = table->GetValue(row, col);
      std::cerr << "row " << row << ", col " << col << " - " << v.ToString() << " ("
                << vtkImageScalarTypeNameMacro(v.GetType()) << ")" << std::endl;
    }
  }

  query->SetQuery("DROP TABLE people");
  if (!query->Execute())
  {
    std::cerr << "DROP TABLE people query failed" << std::endl;
    reader->Delete();
    query->Delete();
    db->Delete();
    return 1;
  }

  reader->Delete();
  query->Delete();
  db->Delete();

  // ----------------------------------------------------------------------
  // Testing transformation of a schema into a PostgreSQL database

  // 1. Create the schema
  DatabaseSchemaWith2Tables schema;

  // 2. Convert the schema into a PostgreSQL database
  std::cerr << "@@ Converting the schema into a PostgreSQL database...";

  db = vtkPostgreSQLDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL(VTK_PSQL_TEST_URL));
  status = db->Open();

  if (!status)
  {
    std::cerr << "Couldn't open database.\nError: \"" << db->GetLastErrorText() << "\"\n";
    db->Delete();
    return 1;
  }

  status = db->EffectSchema(schema.GetSchema());
  if (!status)
  {
    std::cerr << "Could not effect test schema.\n";
    db->Delete();
    return 1;
  }
  std::cerr << " done." << std::endl;

  // 3. Count tables of the newly created database
  std::cerr << "@@ Counting tables of the newly created database... ";

  query = db->GetQueryInstance();
  query->SetQuery("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public'");
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::vector<std::string> tables;
  while (query->NextRow())
  {
    tables.push_back(query->DataValue(0).ToString());
  }

  int numTbl = tables.size();

  if (numTbl != schema->GetNumberOfTables())
  {
    std::cerr << "Found an incorrect number of tables: " << numTbl
              << " != " << schema->GetNumberOfTables() << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << numTbl << " found.\n";

  // 4. Inspect these tables
  std::cerr << "@@ Inspecting these tables..."
            << "\n";
  int tblHandle = schema.GetTableBHandle();
  std::string queryStr;
  for (tblHandle = 0; tblHandle < numTbl; ++tblHandle)
  {
    std::string tblName(schema->GetTableNameFromHandle(tblHandle));
    std::cerr << "   Table: " << tblName << "\n";

    if (tblName != tables[tblHandle])
    {
      std::cerr << "Fetched an incorrect name: " << tables[tblHandle] << " != " << tblName
                << std::endl;
      query->Delete();
      db->Delete();
      return 1;
    }

    // Check columns
    queryStr = "SELECT column_name FROM information_schema.columns WHERE table_schema = 'public' "
               "AND table_name = '";
    queryStr += tblName;
    queryStr += "' order by ordinal_position";
    query->SetQuery(queryStr);
    if (!query->Execute())
    {
      std::cerr << "Query failed" << std::endl;
      query->Delete();
      db->Delete();
      return 1;
    }

    int numFields = query->GetNumberOfFields();
    int colHandle = 0;
    for (; query->NextRow(); ++colHandle)
    {
      for (int field = 0; field < numFields; ++field)
      {
        if (field)
        {
          std::cerr << ", ";
        }
        else // if ( field )
        {
          std::string colName(schema->GetColumnNameFromHandle(tblHandle, colHandle));
          if (colName != query->DataValue(field).ToString())
          {
            std::cerr << "Found an incorrect column name: " << query->DataValue(field).ToString()
                      << " != " << colName << std::endl;
            query->Delete();
            db->Delete();
            return 1;
          }
          std::cerr << "     Column: ";
        }
        std::cerr << query->DataValue(field).ToString();
      }
      std::cerr << std::endl;
    }

    if (colHandle != schema->GetNumberOfColumnsInTable(tblHandle))
    {
      std::cerr << "Found an incorrect number of columns: " << colHandle
                << " != " << schema->GetNumberOfColumnsInTable(tblHandle) << std::endl;
      query->Delete();
      db->Delete();
      return 1;
    }
  }

  // 5. Populate these tables using the trigger mechanism
  std::cerr << "@@ Populating table atable...";

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Bas-Rhin', 67 )";
  query->SetQuery(queryStr);
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Hautes-Pyrenees', 65 )";
  query->SetQuery(queryStr);
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Vosges', 88 )";
  query->SetQuery(queryStr);
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << " done." << std::endl;

  // 6. Check that the trigger-dependent table has indeed been populated
  std::cerr << "@@ Checking trigger-dependent table btable...\n";

  queryStr = "SELECT somevalue FROM btable ORDER BY somevalue DESC";
  query->SetQuery(queryStr);
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << "   Entries in column somevalue of table btable, in descending order:\n";
  static const char* dpts[] = { "88", "67", "65" };
  int numDpt = 0;
  for (; query->NextRow(); ++numDpt)
  {
    if (query->DataValue(0).ToString() != dpts[numDpt])
    {
      std::cerr << "Found an incorrect value: " << query->DataValue(0).ToString()
                << " != " << dpts[numDpt] << std::endl;
      query->Delete();
      db->Delete();
      return 1;
    }
    std::cerr << "     " << query->DataValue(0).ToString() << "\n";
  }

  if (numDpt != 3)
  {
    std::cerr << "Found an incorrect number of entries: " << numDpt << " != " << 3 << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << " done." << std::endl;

  // 7. Test EscapeString.
  std::cerr << "@@ Escaping a naughty string...";

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( " +
    query->EscapeString(std::string("Str\"ang'eS\ntring"), true) + ", 2 )";
  query->SetQuery(queryStr);
  if (!query->Execute())
  {
    std::cerr << "Query failed" << std::endl;
    query->Delete();
    db->Delete();
    return 1;
  }

  std::cerr << " done." << std::endl;

  // 8. Read back the escaped string to verify it worked.
  std::cerr << "@@ Reading it back... <";

  queryStr = "SELECT somename FROM atable WHERE somenmbr=2";
  query->SetQuery(queryStr);
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

  // 9. Drop tables
  std::cerr << "@@ Dropping these tables...";

  for (std::vector<std::string>::iterator it = tables.begin(); it != tables.end(); ++it)
  {
    queryStr = "DROP TABLE ";
    queryStr += *it;
    query->SetQuery(queryStr);

    if (!query->Execute())
    {
      std::cerr << "Query failed" << std::endl;
      query->Delete();
      db->Delete();
      return 1;
    }
  }

  std::cerr << " done." << std::endl;

  // 10. Delete the database until we run the test again
  std::cerr << "@@ Dropping the database...";

  if (!db->DropDatabase(realDatabase.c_str()))
  {
    std::cout << "Drop of \"" << realDatabase << "\" failed.\n";
    std::cerr << "\"" << db->GetLastErrorText() << "\"" << std::endl;
  }

  std::cerr << " done." << std::endl;

  // Clean up
  db->Delete();
  query->Delete();

  return 0;
}
