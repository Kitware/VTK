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

  // 1. Create the schema

  vtkSQLDatabaseSchema* schema = vtkSQLDatabaseSchema::New();
  schema->SetName( "TestSchema" );

  schema->AddPreamble( "CreateSomeFunction", // by default, the 4-th parameter will be VTK_SQL_ALLBACKENDS
                       "CREATE FUNCTION SomeFunction(integer) RETURNS integer AS $$ SELECT $1; $$ LANGUAGE SQL" ); 

  int tblHandle = 0;
  tblHandle = schema->AddTableMultipleArguments( "ATable",
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
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "InsertTrigger", "DO NOTHING", 
      VTK_SQL_SQLITE,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "InsertTrigger", "FOR EACH ROW EXECUTE PROCEDURE somefunction ()", 
      VTK_SQL_POSTGRESQL,
    vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
      "InsertTrigger", "FOR EACH ROW INSERT INTO BTable SET SomeValue = NEW.SomeNmbr", 
      VTK_SQL_MYSQL,
    vtkSQLDatabaseSchema::END_TABLE_TOKEN
  );

  if ( tblHandle < 0 )
    {
    cerr << "Could not create test schema.\n";
    status = false;
    }
  
  // 2. Check the schema

  // Check the preamble

  int numPre = schema->GetNumberOfPreambles();
  if ( numPre != 1 )
    {
    cerr << "Read " << numPre << " != 1 preamble in test schema.\n";
    status = false;
    }

  for ( int preHandle = 0; preHandle < numPre; ++ preHandle )
    {
    vtkStdString preName = schema->GetPreambleNameFromHandle( preHandle );
    if ( preName != "CreateSomeFunction" )
      {
      cerr << "Could not retrieve preamble name CreateSomeFunction from test schema.\n";
      status = false;
      }
    else
      {
      cerr << "Preamble name: " 
           << preName
           << "\n";
      }
    
    vtkStdString preAction = schema->GetPreambleActionFromHandle( preHandle );
    if ( preAction != "CREATE FUNCTION SomeFunction(integer) RETURNS integer AS $$ SELECT $1; $$ LANGUAGE SQL" )
      {
      cerr << "Could not retrieve preamble action CREATE FUNCTION SomeFunction(integer) RETURNS integer AS $$ SELECT $1; $$ LANGUAGE SQL from test schema.\n";
      status = false;
      }
    else
      {
      cerr << "Preamble Action: " 
           << preAction
           << "\n";
      }
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

  // Loop over all columns of the table
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
  vtkstd::multiset<vtkStdString> trgNames;
  trgNames.insert( vtkStdString ( "InsertTrigger" ) );
  trgNames.insert( vtkStdString ( "InsertTrigger" ) );
  trgNames.insert( vtkStdString ( "InsertTrigger" ) );

  vtkstd::multiset<int> trgTypes;
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );

  vtkstd::multiset<vtkStdString> trgActions;
  trgActions.insert( vtkStdString( "DO NOTHING" ) );
  trgActions.insert( vtkStdString( "FOR EACH ROW INSERT INTO BTable SET SomeValue = NEW.SomeNmbr" ) );
  trgActions.insert( vtkStdString( "FOR EACH ROW EXECUTE PROCEDURE somefunction ()" ) );

  vtkstd::multiset<vtkStdString> trgBackends;
  trgBackends.insert( vtkStdString( VTK_SQL_MYSQL ) );
  trgBackends.insert( vtkStdString( VTK_SQL_SQLITE ) );
  trgBackends.insert( vtkStdString( VTK_SQL_POSTGRESQL ) );

  // Loop over all triggers of the previously created table
  int numTrg = schema->GetNumberOfTriggersInTable( tblHandle );
  if ( numTrg != 3 )
    {
    cerr << "Read " << numTrg << " != 3 triggers in test schema.\n";
    status = false;
    }
  
  for ( int trgHandle = 0; trgHandle < numTrg; ++ trgHandle )
    {
    vtkStdString trgName = schema->GetTriggerNameFromHandle( tblHandle, trgHandle );
    cerr << "Trigger name: " 
         << trgName
         << "\n";

    vtkstd::multiset<vtkStdString>::iterator sit = trgNames.find( trgName );
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

    vtkstd::multiset<int>::iterator iit = trgTypes.find( trgType );
    if ( iit != trgTypes.end() )
      {
      trgTypes.erase ( iit );
      }
    else
      {
      cerr << "Could not retrieve trigger type " << trgType  << " from test schema.\n";
      status = false;
      }

    vtkStdString trgAction = schema->GetTriggerActionFromHandle( tblHandle, trgHandle );
    cerr << "Trigger action: " 
         << trgAction
         << "\n";

    sit = trgActions.find( trgAction );
    if ( sit != trgActions.end() )
      {
      trgActions.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve trigger action " << trgAction  << " from test schema.\n";
      status = false;
      }

    vtkStdString trgBackend = schema->GetTriggerBackendFromHandle( tblHandle, trgHandle );
    cerr << "Trigger backend: " 
         << trgBackend
         << "\n";

    sit = trgBackends.find( trgBackend );
    if ( sit != trgBackends.end() )
      {
      trgBackends.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve trigger backend " << trgBackend  << " from test schema.\n";
      status = false;
      }
    }

  schema->Delete();

  return status ? 0 : 1;
}
