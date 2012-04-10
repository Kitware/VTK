/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPostgreSQLDatabase.cxx

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
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "vtkPostgreSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkStringArray.h"
#include "vtkToolkits.h"
#include "DatabaseSchemaWith2Tables.h"
#include "vtkIOPostgresSQLTestingCxxConfigure.h"
#include <vector>

int TestPostgreSQLDatabase( int /*argc*/, char* /*argv*/[] )
{
  // This test requires the user in VTK_PSQL_TEST_URL to have
  // permission to create and drop the database named in that URL
  // as well as tables in that database. That user must also be
  // able to connect to the "template1" database (which initdb
  // creates and should be present on all systems -- we do NOT
  // support non-standard configurations where this is not true).
  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast(
    vtkSQLDatabase::CreateFromURL( VTK_PSQL_TEST_URL ) );
  vtkStdString realDatabase = db->GetDatabaseName();
  db->SetDatabaseName( "template1" ); // This is guaranteed to exist
  bool status = db->Open();
  if ( ! status )
    {
    cerr
      << "Couldn't open database.\nError message: \""
      << db->GetLastErrorText() << "\"\n";
    db->Delete();
    return 1;
    }

  vtkStringArray* dbNames = db->GetDatabases();
  cout << "Database list:\n";
  for ( int dbi = 0; dbi < dbNames->GetNumberOfValues(); ++ dbi )
    {
    cout << "+ " << dbNames->GetValue( dbi ) << endl;
    }
  dbNames->Delete();
  if ( ! db->CreateDatabase( realDatabase.c_str(), true ) )
    {
    cerr << "Error: " << db->GetLastErrorText() << endl;
    }

  vtkSQLQuery* query = db->GetQueryInstance();

  // Force a database connection close
  // This also forces us to connect to the database named in the test URL.
  vtkStdString fauxDatabase = realDatabase + "blarney";
  db->SetDatabaseName( fauxDatabase.c_str() );
  db->SetDatabaseName( realDatabase.c_str() );

  if (!db->Open())
    {
    cerr << "Error: " << db->GetLastErrorText() << endl;
    }

  // Test that bad queries fail without segfaulting...
  vtkStdString dropQuery( "DROP TABLE people" );
  cout << dropQuery << endl;
  query->SetQuery( dropQuery.c_str() );
  if ( ! query->Execute() )
    {
    cout << "Drop query did not succeed (this result *** was *** expected). The last message: " << endl;
    cout << "   " << query->GetLastErrorText() << endl;
    }
  else
    {
    cerr << "The query \"DROP TABLE people\" succeeded when it should not have.\n";
    }

  // Test table creation, insertion, queries
  vtkStdString createQuery( "CREATE TABLE people (name TEXT, age INTEGER, weight FLOAT)" );
  cout << createQuery << endl;
  query->SetQuery( createQuery.c_str() );
  if ( ! query->Execute() )
    {
    cerr << "Create query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  for ( int i = 0; i < 40; ++ i )
    {
    char insertQuery[200];
    sprintf( insertQuery, "INSERT INTO people VALUES('John Manyjars %d', %d, %d)", i, i, 10 * i );
    cout << insertQuery << endl;
    query->SetQuery( insertQuery );
    if ( ! query->Execute() )
      {
      cerr << "Insert query " << i << " failed" << endl;
      query->Delete();
      db->Delete();
      return 1;
      }
    }


  const char* queryText = "SELECT name, age, weight FROM people WHERE age <= 20";
  query->SetQuery( queryText );
  cerr << endl << "Running query: " << query->GetQuery() << endl;

  cerr << endl << "Using vtkSQLQuery directly to execute query:" << endl;
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  for ( int col = 0; col < query->GetNumberOfFields(); ++ col )
    {
    if ( col > 0 )
      {
      cerr << ", ";
      }
    cerr << query->GetFieldName( col );
    }
  cerr << endl;
  while ( query->NextRow() )
    {
    for ( int field = 0; field < query->GetNumberOfFields(); ++ field )
      {
      if ( field > 0 )
        {
        cerr << ", ";
        }
      cerr << query->DataValue( field ).ToString().c_str();
      }
    cerr << endl;
    }

  cerr << endl << "Using vtkSQLQuery to execute query and retrieve by row:" << endl;
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }
  for ( int col = 0; col < query->GetNumberOfFields(); ++ col )
    {
    if ( col > 0 )
      {
      cerr << ", ";
      }
    cerr << query->GetFieldName( col );
    }
  cerr << endl;
  vtkVariantArray* va = vtkVariantArray::New();
  while ( query->NextRow( va ) )
    {
    for ( int field = 0; field < va->GetNumberOfValues(); ++ field )
      {
      if ( field > 0 )
        {
        cerr << ", ";
        }
      cerr << va->GetValue( field ).ToString().c_str();
      }
    cerr << endl;
    }
  va->Delete();

  cerr << endl << "Using vtkRowQueryToTable to execute query:" << endl;
  vtkRowQueryToTable* reader = vtkRowQueryToTable::New();
  reader->SetQuery( query );
  reader->Update();
  vtkTable* table = reader->GetOutput();
  for ( vtkIdType col = 0; col < table->GetNumberOfColumns(); ++ col )
    {
    table->GetColumn( col )->Print( cerr );
    }
  cerr << endl;
  for ( vtkIdType row = 0; row < table->GetNumberOfRows(); ++ row )
    {
    for ( vtkIdType col = 0; col < table->GetNumberOfColumns(); ++ col )
      {
      vtkVariant v = table->GetValue( row, col );
      cerr << "row " << row << ", col " << col << " - "
        << v.ToString() << " (" << vtkImageScalarTypeNameMacro( v.GetType() ) << ")" << endl;
      }
    }

  query->SetQuery( "DROP TABLE people" );
  if ( ! query->Execute() )
    {
    cerr << "DROP TABLE people query failed" << endl;
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
  cerr << "@@ Converting the schema into a PostgreSQL database...";

  db = vtkPostgreSQLDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( VTK_PSQL_TEST_URL ) );
  status = db->Open();

  if ( ! status )
    {
    cerr
      << "Couldn't open database.\nError: \""
      << db->GetLastErrorText() << "\"\n";
    db->Delete();
    return 1;
    }

  status = db->EffectSchema( schema.GetSchema() );
  if ( ! status )
    {
    cerr << "Could not effect test schema.\n";
    db->Delete();
    return 1;
    }
  cerr << " done." << endl;

  // 3. Count tables of the newly created database
  cerr << "@@ Counting tables of the newly created database... ";

  query = db->GetQueryInstance();
  query->SetQuery( "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public'" );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  std::vector<vtkStdString> tables;
  while ( query->NextRow() )
    {
    tables.push_back( query->DataValue( 0 ).ToString() );
    }

  int numTbl = tables.size();

  if ( numTbl != schema->GetNumberOfTables() )
    {
    cerr << "Found an incorrect number of tables: "
         << numTbl
         << " != "
         << schema->GetNumberOfTables()
         << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << numTbl
       << " found.\n";

  // 4. Inspect these tables
  cerr << "@@ Inspecting these tables..." << "\n";
  int tblHandle = schema.GetTableBHandle();
  vtkStdString queryStr;
  for ( tblHandle = 0; tblHandle < numTbl; ++ tblHandle )
    {
    vtkStdString tblName( schema->GetTableNameFromHandle( tblHandle ) );
    cerr << "   Table: "
         << tblName
         << "\n";

    if ( tblName != tables[tblHandle] )
      {
      cerr << "Fetched an incorrect name: "
           << tables[tblHandle]
           << " != "
           << tblName
           << endl;
      query->Delete();
      db->Delete();
      return 1;
      }

    // Check columns
    queryStr = "SELECT column_name FROM information_schema.columns WHERE table_schema = 'public' AND table_name = '";
    queryStr += tblName;
    queryStr += "' order by ordinal_position";
    query->SetQuery( queryStr );
    if ( ! query->Execute() )
      {
      cerr << "Query failed" << endl;
      query->Delete();
      db->Delete();
      return 1;
      }

    int numFields = query->GetNumberOfFields();
    int colHandle = 0;
    for ( ; query->NextRow(); ++ colHandle )
      {
      for ( int field = 0; field < numFields; ++ field )
        {
        if ( field )
          {
          cerr << ", ";
          }
        else // if ( field )
          {
          vtkStdString colName ( schema->GetColumnNameFromHandle( tblHandle, colHandle ) );
          if ( colName != query->DataValue( field ).ToString() )
            {
            cerr << "Found an incorrect column name: "
                 << query->DataValue( field ).ToString()
                 << " != "
                 << colName
                 << endl;
            query->Delete();
            db->Delete();
            return 1;
            }
          cerr << "     Column: ";
          }
        cerr << query->DataValue( field ).ToString().c_str();
        }
      cerr << endl;
      }

    if ( colHandle != schema->GetNumberOfColumnsInTable( tblHandle ) )
      {
      cerr << "Found an incorrect number of columns: "
           << colHandle
           << " != "
           << schema->GetNumberOfColumnsInTable( tblHandle )
           << endl;
      query->Delete();
      db->Delete();
      return 1;
      }
    }

  // 5. Populate these tables using the trigger mechanism
  cerr << "@@ Populating table atable...";

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Bas-Rhin', 67 )";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Hautes-Pyrenees', 65 )";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  queryStr = "INSERT INTO atable (somename,somenmbr) VALUES ( 'Vosges', 88 )";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << " done." << endl;

  // 6. Check that the trigger-dependent table has indeed been populated
  cerr << "@@ Checking trigger-dependent table btable...\n";

  queryStr = "SELECT somevalue FROM btable ORDER BY somevalue DESC";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << "   Entries in column somevalue of table btable, in descending order:\n";
  static const char *dpts[] = { "88", "67", "65" };
  int numDpt = 0;
  for ( ; query->NextRow(); ++ numDpt )
    {
    if ( query->DataValue( 0 ).ToString() != dpts[numDpt] )
      {
      cerr << "Found an incorrect value: "
           << query->DataValue( 0 ).ToString()
           << " != "
           << dpts[numDpt]
           << endl;
      query->Delete();
      db->Delete();
      return 1;
      }
    cerr << "     "
         << query->DataValue( 0 ).ToString()
         << "\n";
    }

  if ( numDpt != 3 )
    {
    cerr << "Found an incorrect number of entries: "
         << numDpt
         << " != "
         << 3
         << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << " done." << endl;

  // 7. Test EscapeString.
  cerr << "@@ Escaping a naughty string...";

  queryStr =
    "INSERT INTO atable (somename,somenmbr) VALUES ( " +
    query->EscapeString( vtkStdString( "Str\"ang'eS\ntring" ), true ) +
    ", 2 )";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << " done." << endl;

  // 8. Read back the escaped string to verify it worked.
  cerr << "@@ Reading it back... <";

  queryStr = "SELECT somename FROM atable WHERE somenmbr=2";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  if ( ! query->NextRow() )
    {
    cerr << "Query returned no results" << endl;
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << query->DataValue( 0 ).ToString().c_str() << "> ";
  cerr << " done." << endl;

  // 9. Drop tables
  cerr << "@@ Dropping these tables...";

  for ( std::vector<vtkStdString>::iterator it = tables.begin();
        it != tables.end(); ++ it )
    {
    queryStr = "DROP TABLE ";
    queryStr += *it;
    query->SetQuery( queryStr );

    if ( ! query->Execute() )
      {
      cerr << "Query failed" << endl;
      query->Delete();
      db->Delete();
      return 1;
      }
    }

  cerr << " done." << endl;

  // 10. Delete the database until we run the test again
  cerr << "@@ Dropping the database...";

  if ( ! db->DropDatabase( realDatabase.c_str() ) )
    {
    cout << "Drop of \"" << realDatabase.c_str() << "\" failed.\n";
    cerr << "\"" << db->GetLastErrorText() << "\"" << endl;
    }

  cerr << " done." << endl;

  // Clean up
  db->Delete();
  query->Delete();

  return 0;
}
