/*=========================================================================

Program:   Visualization Toolkit
Module:    TestSQLDatabaseSchema.cxx

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
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this test.

#include "vtkSQLDatabaseSchema.h"

#include "vtkStdString.h"

#include <vtkstd/set>
 
int TestSQLDatabaseSchema( int /*argc*/, char* /*argv*/[] )
{
  bool status = true;
  int tblHandle = 0;
  vtkSQLDatabaseSchema* schema = vtkSQLDatabaseSchema::New();

  schema->SetName( "TestSchema" );
  tblHandle = schema->AddTableMultipleArguments( "StrangeTable",
                                                 vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::SERIAL,  "TableKey",  0, "",
                                                 vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::VARCHAR, "SomeName", 11, "NOT NULL",
                                                 vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "SomeNmbr", 17, "DEFAULT 0",
                                                 vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::PRIMARY_KEY, "BigKey",
                                                 vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "TableKey",
                                                 vtkSQLDatabaseSchema::END_INDEX_TOKEN,
                                                 vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::UNIQUE, "ReverseLookup",
                                                 vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "SomeName",
                                                 vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "SomeNmbr",
                                                 vtkSQLDatabaseSchema::END_INDEX_TOKEN,
                                                 vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT, "InsertTrigger", "INSERT INTO OtherTable ( Value ) VALUES NEW.SomeNmbr",
                                                 vtkSQLDatabaseSchema::END_TABLE_TOKEN
                                                 );

  if ( tblHandle < 0 )
    {
    cerr << "Could not create test schema.\n";
    status = false;
    }
  
  // Define the correct (reference) columns and types
  vtkstd::set<vtkStdString> colNames;
  colNames.insert( vtkStdString ( "SomeNmbr" ) );
  colNames.insert( vtkStdString ( "SomeName" ) );
  colNames.insert( vtkStdString ( "TableKey" ) );
  vtkstd::set<int> colTypes;
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::BIGINT ) );
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::SERIAL ) );
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::VARCHAR ) );

  // Loop over all columns of the previously created table
  int numCol = schema->GetNumberOfColumnsInTable( tblHandle );
  if ( numCol != 3 )
    {
    cerr << "Read " << numCol << " != 3 columns in test schema.\n";
    status = false;
    }

  for ( int colHandle = 0; colHandle < numCol; ++ colHandle )
    {
    vtkStdString colName = schema->GetColumnNameFromHandle( tblHandle, colHandle );
    cerr << "Column name: " 
         << colName
         << "\n";

    vtkstd::set<vtkStdString>::iterator sit = colNames.find( colName );
    if ( sit != colNames.end() )
      {
      colNames.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve column name " << colName  << " from test schema.\n";
      status = false;
      }

    int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle );
    cerr << "Column type: " 
         << colType
         << "\n";

    vtkstd::set<int>::iterator iit = colTypes.find( colType );
    if ( iit != colTypes.end() )
      {
      colTypes.erase ( iit );
      }
    else
      {
      cerr << "Could not retrieve column type " << colType  << " from test schema.\n";
      status = false;
      }
    }

  // Define the correct (reference) indices and types
  vtkstd::set<vtkStdString> idxNames;
  idxNames.insert( vtkStdString ( "BigKey" ) );
  idxNames.insert( vtkStdString ( "ReverseLookup" ) );
  vtkstd::set<int> idxTypes;
  idxTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::PRIMARY_KEY ) );
  idxTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::UNIQUE ) );

  // Loop over all indices of the previously created table
  int numIdx = schema->GetNumberOfIndicesInTable( tblHandle );
  if ( numIdx != 2 )
    {
    cerr << "Read " << numIdx << " != 2 indices in test schema.\n";
    status = false;
    }
  
  for ( int idxHandle = 0; idxHandle < numIdx; ++ idxHandle )
    {
    vtkStdString idxName = schema->GetIndexNameFromHandle( tblHandle, idxHandle );
    cerr << "Index name: " 
         << idxName
         << "\n";

    vtkstd::set<vtkStdString>::iterator sit = idxNames.find( idxName );
    if ( sit != idxNames.end() )
      {
      idxNames.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve index name " << idxName  << " from test schema.\n";
      status = false;
      }

    int idxType = schema->GetIndexTypeFromHandle( tblHandle, idxHandle );
    cerr << "Index type: " 
         << idxType
         << "\n";

    vtkstd::set<int>::iterator iit = idxTypes.find( idxType );
    if ( iit != idxTypes.end() )
      {
      idxTypes.erase ( iit );
      }
    else
      {
      cerr << "Could not retrieve index type " << idxType  << " from test schema.\n";
      status = false;
      }
    }

  // Define the correct (reference) triggers and types
  vtkstd::set<vtkStdString> trgNames;
  trgNames.insert( vtkStdString ( "InsertTrigger" ) );
  vtkstd::set<int> trgTypes;
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );

  // Loop over all triggers of the previously created table
  int numTrg = schema->GetNumberOfTriggersInTable( tblHandle );
  if ( numTrg != 1 )
    {
    cerr << "Read " << numTrg << " != 2 triggers in test schema.\n";
    status = false;
    }
  
  for ( int trgHandle = 0; trgHandle < numTrg; ++ trgHandle )
    {
    vtkStdString trgName = schema->GetTriggerNameFromHandle( tblHandle, trgHandle );
    cerr << "Trigger name: " 
         << trgName
         << "\n";

    vtkstd::set<vtkStdString>::iterator sit = trgNames.find( trgName );
    if ( sit != trgNames.end() )
      {
      trgNames.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve trigger name " << trgName  << " from test schema.\n";
      status = false;
      }

    int trgType = schema->GetTriggerTypeFromHandle( tblHandle, trgHandle );
    cerr << "Trigger type: " 
         << trgType
         << "\n";

    vtkstd::set<int>::iterator iit = trgTypes.find( trgType );
    if ( iit != trgTypes.end() )
      {
      trgTypes.erase ( iit );
      }
    else
      {
      cerr << "Could not retrieve trigger type " << trgType  << " from test schema.\n";
      status = false;
      }
    }

  schema->Delete();

  return status ? 0 : 1;
  return 0;
}
