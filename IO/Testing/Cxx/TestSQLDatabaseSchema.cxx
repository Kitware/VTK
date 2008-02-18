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
  vtkstd::set<vtkStdString> colRefs;
  colRefs.insert( vtkStdString ( "SomeNmbr" ) );
  colRefs.insert( vtkStdString ( "SomeName" ) );
  colRefs.insert( vtkStdString ( "TableKey" ) );
  vtkstd::set<int> typRefs;
  typRefs.insert( static_cast<int>( vtkSQLDatabaseSchema::BIGINT ) );
  typRefs.insert( static_cast<int>( vtkSQLDatabaseSchema::SERIAL ) );
  typRefs.insert( static_cast<int>( vtkSQLDatabaseSchema::VARCHAR ) );

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

    vtkstd::set<vtkStdString>::iterator sit = colRefs.find( colName );
    if ( sit != colRefs.end() )
      {
      colRefs.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve column name " << colName  << " in test schema.\n";
      status = false;
      }

    int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle );
    cerr << "Column type: " 
         << colType
         << "\n";

    vtkstd::set<int>::iterator iit = typRefs.find( colType );
    if ( iit != typRefs.end() )
      {
      typRefs.erase ( iit );
      }
    else
      {
      cerr << "Could not retrieve column type " << colType  << " in test schema.\n";
      status = false;
      }
    }

  // Loop over all indices of the previously created table
  int numIdx = schema->GetNumberOfIndicesInTable( tblHandle );
  if ( numIdx != 2 )
    {
    cerr << "Read " << numIdx << " != 2 indices in test schema.\n";
    status = false;
    }
  
  for ( int idxHandle = 0; idxHandle < numIdx; ++ idxHandle )
    {
    cerr << "Index name: " 
         << schema->GetIndexNameFromHandle( tblHandle, idxHandle )
         << "\n";
    }

  // Loop over all triggers of the previously created table
  int numTrg = schema->GetNumberOfTriggersInTable( tblHandle );
  if ( numTrg != 1 )
    {
    cerr << "Read " << numTrg << " != 2 triggers in test schema.\n";
    status = false;
    }
  
  for ( int trgHandle = 0; trgHandle < numTrg; ++ trgHandle )
    {
    cerr << "Trigger name: " 
         << schema->GetTriggerNameFromHandle( tblHandle, trgHandle )
         << "\n";
    }

  schema->Delete();

  return status ? 0 : 1;
  return 0;
}
