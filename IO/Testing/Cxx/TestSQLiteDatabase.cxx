/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSQLiteDatabase.cxx

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

#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkRowQueryToTable.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtkstd/vector>

int TestSQLiteDatabase( int /*argc*/, char* /*argv*/[])
{
  bool status;

  cerr << ">>>>> Testing creation modes." << endl;

  vtkSQLiteDatabase* db1 = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://local.db" ) );
  status = db1->Open("", vtkSQLiteDatabase::CREATE_OR_CLEAR);
  vtkSQLQuery* query1 = db1->GetQueryInstance();
  query1->SetQuery("CREATE TABLE test (id INTEGER)");
  if (!query1->Execute())
    {
      cerr << "Create query failed" << endl;
      return 1;
    }
  if ( ! status )
    {
    cerr << "Couldn't open database using CREATE_OR_CLEAR.\n";
    return 1;
    }
  db1->Delete();
  query1->Delete();

  vtkSQLiteDatabase* db2 = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://local.db" ) );
  status = db2->Open("", vtkSQLiteDatabase::CREATE);
  if ( status )
    {
    cerr << "Using CREATE on an existing file should have failed but did not.\n";
    return 1;
    }
  db2->Delete();

  vtkSQLiteDatabase* db3 = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://local.db" ) );
  status = db3->Open("", vtkSQLiteDatabase::USE_EXISTING_OR_CREATE);
  if ( !status )
    {
    cerr << "Using USE_EXISTING_OR_CREATE did not work.\n";
    return 1;
    }
  vtkSQLQuery* query3 = db3->GetQueryInstance();
  query3->SetQuery("SELECT * from test");
  if (!query3->Execute())
    {
      cerr << "Select query failed" << endl;
      return 1;
    }
  db3->Delete();
  query3->Delete();

  vtkSQLiteDatabase* db4 = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://local.db" ) );
  status = db4->Open("", vtkSQLiteDatabase::CREATE_OR_CLEAR);
  if ( !status )
    {
    cerr << "Using CREATE_OR_CLEAR did not work.\n";
    return 1;
    }
  vtkSQLQuery* query4 = db4->GetQueryInstance();
  query4->SetQuery("SELECT * from test");
  if (query4->Execute())
    {
      cerr << "Select query succeeded when it shouldn't have." << endl;
      return 1;
    }
  db4->Delete();
  query4->Delete();

  cerr << ">>>>> Testing database functions" << endl;

  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://:memory:" ) );
  status = db->Open("");

  if ( ! status )
    {
    cerr << "Couldn't open database.\n";
    return 1;
    }

  vtkSQLQuery* query = db->GetQueryInstance();

  vtkStdString createQuery("CREATE TABLE IF NOT EXISTS people (name TEXT, age INTEGER, weight FLOAT)");
  cout << createQuery << endl;
  query->SetQuery( createQuery.c_str());
  if (!query->Execute())
    {
      cerr << "Create query failed" << endl;
      return 1;
    }
  
  int i;
  for ( i = 0; i < 20; i++)
    {
      char insertQuery[200];
      sprintf( insertQuery, "INSERT INTO people (name, age, weight) VALUES('John Doe %d', %d, %f)",
        i, i, 10.1*i );
      cout << insertQuery << endl;
      query->SetQuery( insertQuery );
      if (!query->Execute())
        {
        cerr << "Insert query " << i << " failed" << endl;
        return 1;
        }
    }

  
  const char *placeholders = "INSERT INTO people (name, age, weight) VALUES (?, ?, ?)";
  query->SetQuery(placeholders);
  for ( i = 21; i < 40; i++ )
    {
    char name[20];
    sprintf(name, "John Doe %d", i);
    bool bind1 = query->BindParameter(0, name);
    bool bind2 = query->BindParameter(1, i);
    bool bind3 = query->BindParameter(2, 10.1*i);
    if (!(bind1 && bind2 && bind3))
      {
      cerr << "Parameter binding failed on query " << i
           << ": " << bind1 << " " << bind2 << " " << bind3 << endl;
      return 1;
      }
    cout << query->GetQuery() << endl;
    if (!query->Execute())
      {
      cerr << "Insert query " << i << " failed" << endl;
      return 1;
      }
    }

  const char *queryText = "SELECT name, age, weight FROM people WHERE age <= 20";
  query->SetQuery( queryText );
  cerr << endl << "Running query: " << query->GetQuery() << endl;

  cerr << endl << "Using vtkSQLQuery directly to execute query:" << endl;
  if (!query->Execute())
    {
      cerr << "Query failed" << endl;
      return 1;
    }

  for ( int col = 0; col < query->GetNumberOfFields(); col++)
    {
    if ( col > 0)
      {
      cerr << ", ";
      }
    cerr << query->GetFieldName( col );
    }
  cerr << endl;
  while ( query->NextRow())
    {
    for ( int field = 0; field < query->GetNumberOfFields(); field++)
      {
      if ( field > 0)
        {
        cerr << ", ";
        }
      cerr << query->DataValue( field ).ToString().c_str();
      }
    cerr << endl;
    }
  
  cerr << endl << "Using vtkSQLQuery to execute query and retrieve by row:" << endl;
  if (!query->Execute())
    {
      cerr << "Query failed" << endl;
      return 1;
    }
  for ( int col = 0; col < query->GetNumberOfFields(); col++)
    {
    if ( col > 0)
      {
      cerr << ", ";
      }
    cerr << query->GetFieldName( col );
    }
  cerr << endl;
  vtkVariantArray* va = vtkVariantArray::New();
  while ( query->NextRow( va ))
    {
    for ( int field = 0; field < va->GetNumberOfValues(); field++)
      {
      if ( field > 0)
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
  for ( vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
    {
    table->GetColumn( col )->Print( cerr );
    }
  cerr << endl;
  for ( vtkIdType row = 0; row < table->GetNumberOfRows(); row++)
    {
    for ( vtkIdType col = 0; col < table->GetNumberOfColumns(); col++)
      {
      vtkVariant v = table->GetValue( row, col );
      cerr << "row " << row << ", col " << col << " - "
        << v.ToString() << " (" << vtkImageScalarTypeNameMacro( v.GetType()) << ")" << endl;
      }
    }

  reader->Delete();
  query->Delete();
  db->Delete();

// ----------------------------------------------------------------------
// Testing transformation of a schema into a SQLite database

  // 1. Create the schema
#include "DatabaseSchemaWith2Tables.cxx"

  // 2. Convert the schema into a SQLite database
  cerr << "@@ Converting the schema into a SQLite database...";

  vtkSQLiteDatabase* dbSch = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://:memory:" ) );
  status = dbSch->Open("");

  if ( ! status )
    {
    cerr << "Couldn't open database.\n";
    return 1;
    }

  status = dbSch->EffectSchema( schema ); 
  if ( ! status )
    {
    cerr << "Could not effect test schema.\n";
    return 1;
    }
  cerr << " done." << endl;

  // 3. Count tables of the newly created database
  cerr << "@@ Fetching table names of the newly created database:\n";

  query = dbSch->GetQueryInstance();

  query->SetQuery( "SELECT name FROM sqlite_master WHERE type = \"table\"" );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    return 1;
    }

  vtkstd::vector<vtkStdString> tables;
  for ( tblHandle = 0; query->NextRow(); ++ tblHandle )
    {
    vtkStdString tblNameSch( schema->GetTableNameFromHandle( tblHandle ) );
    vtkStdString tblNameDB( query->DataValue( 0 ).ToString() );
    cerr << "     " 
         << tblNameDB
         << "\n";

    if ( tblNameDB != tblNameSch )
      {
      cerr << "Fetched an incorrect name: " 
           << tblNameDB
           << " != " 
           << tblNameSch
           << endl;
      return 1;
      }

    tables.push_back( tblNameDB );
    }

  if ( tblHandle != schema->GetNumberOfTables() )
    {
    cerr << "Found an incorrect number of tables: " 
         << tblHandle 
         << " != " 
         << schema->GetNumberOfTables()
         << endl;
    return 1;
    }
  
  cerr << "   "
       << tblHandle
       << " found.\n";

  // 4. Test EscapeString.
  cerr << "@@ Escaping a naughty string...";

  vtkStdString queryStr =
    "INSERT INTO atable (somename,somenmbr) VALUES ( " +
    query->EscapeString( vtkStdString( "Str\"ang'eS\ntring" ), true ) +
    ", 2 )";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    schema->Delete();
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << " done." << endl;

  // 5. Read back the escaped string to verify it worked.
  cerr << "@@ Reading it back... <";

  queryStr = "SELECT somename FROM atable WHERE somenmbr=2";
  query->SetQuery( queryStr );
  if ( ! query->Execute() )
    {
    cerr << "Query failed" << endl;
    schema->Delete();
    query->Delete();
    db->Delete();
    return 1;
    }

  if ( ! query->NextRow() )
    {
    cerr << "Query returned no results" << endl;
    schema->Delete();
    query->Delete();
    db->Delete();
    return 1;
    }

  cerr << query->DataValue( 0 ).ToString().c_str() << "> ";
  cerr << " done." << endl;

  // 6. Drop tables
  cerr << "@@ Dropping these tables...";

  for ( vtkstd::vector<vtkStdString>::iterator it = tables.begin();
        it != tables.end(); ++ it )
    {
    queryStr = "DROP TABLE ";
    queryStr += *it;
    query->SetQuery( queryStr );

    if ( ! query->Execute() )
      {
      cerr << "Query failed" << endl;
      return 1;
      }
    }

  cerr << " done." << endl;

  // Clean up
  dbSch->Delete();
  schema->Delete();
  query->Delete();

  return 0;
}
