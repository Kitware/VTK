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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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
#include "DatabaseSchemaWith2Tables.cxx"

  // 2. Check the schema

  // Define the correct (reference) columns and types
  vtkstd::set<vtkStdString> preNames;
  preNames.insert( vtkStdString( "dropplpgsql" ) );
  preNames.insert( vtkStdString( "loadplpgsql" ) );
  preNames.insert( vtkStdString( "createsomefunction" ) );
  vtkstd::multiset<vtkStdString> preBackends;
  preBackends.insert( vtkStdString( VTK_SQL_POSTGRESQL ) );
  preBackends.insert( vtkStdString( VTK_SQL_POSTGRESQL ) );
  preBackends.insert( vtkStdString( VTK_SQL_POSTGRESQL ) );

  // Loop over all preambles
  int numPre = schema->GetNumberOfPreambles();
  if ( numPre != 3 )
    {
    cerr << "Read " << numPre << " != 3 preamble in test schema.\n";
    status = false;
    }

  for ( int preHandle = 0; preHandle < numPre; ++ preHandle )
    {
    vtkStdString preName = schema->GetPreambleNameFromHandle( preHandle );
    cerr << "Preamble name: " 
         << preName
         << "\n";

    vtkstd::set<vtkStdString>::iterator sit = preNames.find( preName );
    if ( sit != preNames.end() )
      {
      preNames.erase ( sit );
      }
    else
      {
      cerr << "Could not retrieve preamble name " << preName  << " from test schema.\n";
      status = false;
      }

    vtkStdString preBackend = schema->GetPreambleBackendFromHandle( preHandle );
    cerr << "Preamble backend: " 
         << preBackend
         << "\n";

    vtkstd::multiset<vtkStdString>::iterator mit = preBackends.find( preBackend );
    if ( mit != preBackends.end() )
      {
      preBackends.erase ( mit );
      }
    else
      {
      cerr << "Could not retrieve preamble backend " << preBackend  << " from test schema.\n";
      status = false;
      }
    }

  // Define the correct (reference) columns and types
  vtkstd::set<vtkStdString> colNames;
  colNames.insert( vtkStdString( "somenmbr" ) );
  colNames.insert( vtkStdString( "somename" ) );
  colNames.insert( vtkStdString( "tablekey" ) );
  vtkstd::set<int> colTypes;
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::BIGINT ) );
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::SERIAL ) );
  colTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::VARCHAR ) );

  // Loop over all columns of the first table
  tblHandle = 0;
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
  idxNames.insert( vtkStdString ( "bigkey" ) );
  idxNames.insert( vtkStdString ( "reverselookup" ) );
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
  trgNames.insert( vtkStdString ( "inserttrigger" ) );
  trgNames.insert( vtkStdString ( "inserttrigger" ) );
  trgNames.insert( vtkStdString ( "inserttrigger" ) );

  vtkstd::multiset<int> trgTypes;
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );
  trgTypes.insert( static_cast<int>( vtkSQLDatabaseSchema::AFTER_INSERT ) );

  vtkstd::multiset<vtkStdString> trgActions;
  trgActions.insert( vtkStdString( "DO NOTHING" ) );
  trgActions.insert( vtkStdString( "FOR EACH ROW INSERT INTO btable SET somevalue = NEW.somenmbr" ) );
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
