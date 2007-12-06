/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPostgreSQLDatabase.cxx

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
#include "vtkPostgreSQLDatabase.h"
#include "vtkPostgreSQLDatabasePrivate.h"

#include "vtkObjectFactory.h"
#include "vtkPostgreSQLQuery.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <pqxx/pqxx>

vtkStandardNewMacro(vtkPostgreSQLDatabase);
vtkCxxRevisionMacro(vtkPostgreSQLDatabase, "1.1");

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::vtkPostgreSQLDatabase()
{
  this->URL = 0;
  this->Connection = 0;
  this->URLMTime = this->MTime;
  this->ConnectionMTime = this->MTime;
}

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::~vtkPostgreSQLDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }

  this->SetURL( 0 );
}

// ----------------------------------------------------------------------
void vtkPostgreSQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Connection: ";
  if ( this->Connection )
    {
    os << this->Connection << endl;
    }
  else
    {
    os << "(null)" << endl;
    }
  os << indent << "URL: " << ( this->URL ? this->URL : "(null)" ) << endl;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::Open()
{
  if  ( ! this->URL )
    {
    this->SetLastErrorText("Cannot open database because URL is null.");
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
    }

  if ( this->Connection )
    {
    if ( this->ConnectionMTime > this->URLMTime )
      {
      return true; // we already had that database open.
      }
    this->Close(); // close the old connection before opening a new one
    }

  vtkstd::string protocol;
  vtkstd::string username;
  vtkstd::string password;
  vtkstd::string hostname;
  vtkstd::string dataport;
  vtkstd::string database;

  if ( protocol != "psql" ||
       ! vtksys::SystemTools::ParseURL( static_cast<vtkstd::string>( this->URL ),
                                        protocol, username, password,
                                        hostname, dataport, database ) )
    {
    vtkGenericWarningMacro( "Invalid URL: " << this->URL );
    return 0;
    }
  
  vtkstd::string options( "host=" + hostname );

  if ( database.length() ) options += " dbname=" + database;
  
  try
    {
    this->Connection = new vtkPostgreSQLDatabasePrivate( options.c_str() );
    this->ConnectionMTime.Modified();
    }
  catch ( pqxx::sql_error& e )
    {
    this->SetLastErrorText( e.what() );
    return false;
    }

  this->SetLastErrorText( 0 );
  return true;
}

// ----------------------------------------------------------------------
void vtkPostgreSQLDatabase::Close()
{
  if ( this->Connection )
    {
    this->Connection->Delete();
    this->Connection = 0;
    this->URLMTime.Modified(); // Force a re-open to occur when Open() is called.
    }
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::IsOpen()
{
  return this->Connection != 0;
}

// ----------------------------------------------------------------------
vtkSQLQuery* vtkPostgreSQLDatabase::GetQueryInstance()
{
  vtkPostgreSQLQuery* query = vtkPostgreSQLQuery::New();
  query->SetDatabase( this );
  return query;
}

// ----------------------------------------------------------------------
const char* vtkPostgreSQLDatabase::GetLastErrorText()
{
  if ( ! this->Connection )
    {
    return "Database is not open.";
    }
  else
    {
    return this->Connection->LastErrorText;
    }
}

// ----------------------------------------------------------------------
vtkStringArray* vtkPostgreSQLDatabase::GetTables()
{
  if ( ! this->Connection )
    {
    // This will always be "Database not open."
    vtkErrorMacro(<< this->GetLastErrorText());
    return 0;
    }

  // NB: Other columns of interest include table_catalog, table_schema, table_type,
  // self_referencing_column_name, reference_generation, user_defined_type_catalog,
  // user_defined_type_schema, user_defined_type_name, is_insertable_into, is_typed,
  // commit_action
  vtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery(
                  "SELECT table_name FROM information_schema.tables"
                  "  WHERE table_schema='public' and table_type='BASE TABLE'" );
  bool status = query->Execute();

  if ( ! status )
    {
    vtkErrorMacro(<< "Database returned error: "
                  << query->GetLastErrorText());
    this->SetLastErrorText( query->GetLastErrorText() );
    query->Delete();
    return 0;
    }
  vtkDebugMacro(<< "GetTables(): SQL query succeeded." );
  vtkStringArray* results = vtkStringArray::New();
  while ( query->NextRow() )
    {
    results->InsertNextValue( query->DataValue( 0 ).ToString() );
    }
  query->Delete();
  this->SetLastErrorText( 0 );
  return results;
}

// ----------------------------------------------------------------------
vtkStringArray* vtkPostgreSQLDatabase::GetRecord( const char* table )
{
  // NB: There are *too many* other column names to list. Even the ones
  // currently in the query below are probably over the top. But there's
  // just so much peanut-buttery goodness in the table, I couldn't resist.
  vtkSQLQuery* query = this->GetQueryInstance();
  vtkStdString text(
                    "SELECT column_name,column_default,data_type,is_nullable,character_maximum_length,numeric_precision,datetime_precision"
                    "  FROM information_schema.columns"
                    "  WHERE table_name='" );
  text += table;
  text += "' ORDER BY ordinal_position";

  query->SetQuery( text.c_str() );
  bool status = query->Execute();
  if ( ! status )
    {
    vtkErrorMacro(<< "GetRecord(" << table << "): Database returned error: "
                  << query->GetLastErrorText());
    this->SetLastErrorText( query->GetLastErrorText() );
    query->Delete();
    return 0;
    }

  // Each row in the results that come back from this query
  // describes a single column in the table.
  vtkStringArray* results = vtkStringArray::New();
    
  while ( query->NextRow() )
    {
    results->InsertNextValue( query->DataValue( 0 ).ToString() );
    }

  query->Delete();
  this->SetLastErrorText( 0 );
  return results;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::IsSupported( int feature )
{
  switch (feature)
    {
    case VTK_SQL_FEATURE_BLOB:
    case VTK_SQL_FEATURE_LAST_INSERT_ID:
    case VTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
    case VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
    case VTK_SQL_FEATURE_PREPARED_QUERIES:
    case VTK_SQL_FEATURE_TRANSACTIONS:
    case VTK_SQL_FEATURE_UNICODE:
    case VTK_SQL_FEATURE_BATCH_OPERATIONS:
    case VTK_SQL_FEATURE_QUERY_SIZE:
      return true;
    default:
      {
      vtkErrorMacro(
                    << "Unknown SQL feature code " << feature << "!  See "
                    << "vtkSQLDatabase.h for a list of possible features.");
      return false;
      };
    }
}

// ----------------------------------------------------------------------
vtkStringArray* vtkPostgreSQLDatabase::GetDatabases()
{
  if ( ! this->Connection )
    {
    vtkErrorMacro( "Must be connected to a server to get a list of databases." );
    return 0;
    }

  vtkSQLQuery* query = this->GetQueryInstance();
  if ( ! query )
    {
    vtkErrorMacro( "Could not create a query." );
    return 0;
    }

  query->SetQuery( "SELECT datname FROM pg_database" );
  if ( ! query->Execute() )
    {
    query->Delete();
    return 0;
    }
  vtkStringArray* dbNames = vtkStringArray::New();
  while ( query->NextRow() )
    {
    dbNames->InsertNextValue( query->DataValue( 0 ).ToString() );
    }
  query->Delete();
  return dbNames;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::CreateDatabase( const char* dbName, bool dropExisting )
{
  if ( ! this->Connection )
    {
    vtkErrorMacro( "Must be connected to a server to create a database." );
    return false;
    }

  if ( dropExisting )
    {
    this->DropDatabase( dbName );
    }

  pqxx::nontransaction work( this->Connection->Connection );
  vtkstd::string qstr( "CREATE DATABASE " );
  qstr += dbName;
  try
    {
    pqxx::result res = work.exec( qstr.c_str() );
    }
  catch ( const vtkstd::exception& e )
    {
    vtkErrorMacro( "Could not create database \"" << dbName << "\". " << this->GetLastErrorText() << "\n" );
    return false;
    }
  this->SetLastErrorText( 0 );
  return true;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::DropDatabase( const char* dbName )
{
  if ( ! this->Connection )
    {
    vtkErrorMacro( "Must be connected to a server to create a database." );
    return false;
    }

  pqxx::nontransaction work( this->Connection->Connection );
  vtkstd::string qstr( "DROP DATABASE " );
  qstr += dbName;
  //qstr += " IF EXISTS";
  try
    {
    pqxx::result res = work.exec( qstr.c_str() );
    }
  catch ( const vtkstd::exception& e )
    {
    this->SetLastErrorText( e.what() );
    return false;
    }
  this->SetLastErrorText( 0 );
  return true;
}
