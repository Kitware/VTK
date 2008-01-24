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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "vtkPostgreSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkStringArray.h"
#include "vtkToolkits.h"

int TestPostgreSQLDatabase( int /*argc*/, char* /*argv*/[] )
{
  // This test requires the user in VTK_PSQL_TEST_URL to have
  // permission to create and drop the database named in that URL
  // as well as tables in that database. That user must also be
  // able to connect to the "template1" database (which initdb
  // creates and should be present on all systems -- we do NOT
  // support non-standard configurations where this is not true).
  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( VTK_PSQL_TEST_URL ) );
  vtkStdString realDatabase = db->GetDatabaseName();
  db->SetDatabaseName( "template1" ); // This is guaranteed to exist
  bool status = db->Open();
  if ( ! status )
    {
    cerr << "Couldn't open database.\n";
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

  // Force a database connection open/close
  // This also forces us to connect to the database named in the test URL.
  vtkStdString fauxDatabase = realDatabase + "blarney";
  db->SetDatabaseName( fauxDatabase.c_str() );
  db->SetDatabaseName( realDatabase.c_str() );

  // Test that bad queries fail without segfaulting...
  vtkStdString dropQuery( "DROP TABLE people" );
  cout << dropQuery << endl;
  query->SetQuery( dropQuery.c_str() );
  if ( ! query->Execute() )
    {
    cout << "Drop query did not succeed (this result was expected). The last message: " << endl;
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

  // Delete the database until we run the test again
  if ( ! db->DropDatabase( realDatabase.c_str() ) )
    {
    cout << "Drop of \"" << realDatabase.c_str() << "\" failed.\n";
    cerr << "\"" << db->GetLastErrorText() << "\"" << endl;
    }
  else
    {
    cout << "Drop of \"" << realDatabase.c_str() << "\" succeeded.\n";
    }

  reader->Delete();
  query->Delete();
  db->Delete();

  return 0;
}
