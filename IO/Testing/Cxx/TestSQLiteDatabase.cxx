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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
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
  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://:memory:" ) );
  bool status = db->Open();

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

  for ( int i = 0; i < 40; i++)
    {
      char insertQuery[200];
      sprintf( insertQuery, "INSERT INTO people VALUES('John Doe %d', %d, %d)",
        i, i, 10*i );
      cout << insertQuery << endl;
      query->SetQuery( insertQuery );
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
  cerr << "@@ Creating a schema...";

  vtkSQLDatabaseSchema* schema = vtkSQLDatabaseSchema::New();
  schema->SetName( "TestSchema" );

  // Insert in alphabetical order so that tables selection does not mix handles
  int tblHandle = schema->AddTableMultipleArguments( "ATable",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::INTEGER, "TableKey",  0, "",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::VARCHAR, "SomeName", 11, "NOT NULL",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "SomeNmbr", 17, "DEFAULT 0",
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::PRIMARY_KEY, "BigKey",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "TableKey",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::UNIQUE, "ReverseLookup",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "SomeName",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "SomeNmbr",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "InsertTrigger", "DO NOTHING", VTK_SQL_SQLIT,
    vtkSQLDatabaseSchema::END_TABLE_TOKEN
  );

  tblHandle = schema->AddTableMultipleArguments( "BTable",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::INTEGER,  "TableKey",  0, "",
    vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "SomeValue", 12, "DEFAULT 0",
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::PRIMARY_KEY, "",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "TableKey",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::UNIQUE, "ReverseLookup",
    vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "SomeValue",
    vtkSQLDatabaseSchema::END_INDEX_TOKEN,
    vtkSQLDatabaseSchema::END_TABLE_TOKEN
  );

  if ( tblHandle < 0 )
    {
    cerr << "Could not create test schema.\n";
    return 1;
    }
  cerr << " done." << endl;
  
  // 2. Convert the schema into a SQLite database
  cerr << "@@ Converting the schema into a SQLite database...";

  vtkSQLiteDatabase* dbSch = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://:memory:" ) );
  status = dbSch->Open();

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

  // 4. Drop tables
  cerr << "@@ Dropping these tables...";

  for ( vtkstd::vector<vtkStdString>::iterator it = tables.begin();
        it != tables.end(); ++ it )
    {
    vtkStdString queryStr ("DROP TABLE " );
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
