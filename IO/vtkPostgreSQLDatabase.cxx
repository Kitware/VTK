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
#include "vtkPostgreSQLQuery.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#define PQXX_ALLOW_LONG_LONG
#include <pqxx/pqxx>

vtkStandardNewMacro(vtkPostgreSQLDatabase);
vtkCxxRevisionMacro(vtkPostgreSQLDatabase, "1.6");

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::vtkPostgreSQLDatabase()
{
  this->Connection = 0;
  this->ConnectionMTime = this->MTime;
  
  this->DatabaseType = 0;
  this->SetDatabaseType("psql");
  this->HostName = 0;
  this->UserName = 0;
  this->Password = 0;
  this->DatabaseName = 0;
  this->ServerPort = -1;
  this->ConnectOptions = 0;
}

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::~vtkPostgreSQLDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }

  this->SetHostName( 0 );
  this->SetUserName( 0 );
  this->SetPassword( 0 );
  this->SetDatabaseName( 0 );
  this->SetConnectOptions( 0 );
  this->SetDatabaseType( 0 );
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
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
  os << indent << "HostName: " << (this->HostName ? this->HostName : "NULL") << endl;
  os << indent << "UserName: " << (this->UserName ? this->UserName : "NULL") << endl;
  os << indent << "Password: " << (this->Password ? this->Password : "NULL") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "NULL") << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "NULL") << endl;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::Open()
{
  if ( ! this->HostName || ! this->DatabaseName )
    {
    //this->SetLastErrorText("Cannot open database because HostName and/or DatabaseName are null.");
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

  vtkstd::string options( "host=" );
  options += this->HostName;
  options += " dbname=";
  options += this->DatabaseName;

  if ( this->ServerPort )
    {
    options += " port=";
    options += this->ServerPort;
    }
  if ( this->UserName && strlen( this->UserName ) > 0 )
    {
    options += " user=";
    options += this->UserName;
    }
  if ( this->Password && strlen( this->Password ) > 0 )
    {
    options += " password=";
    options += this->Password;
    }
  if ( this->ConnectOptions && strlen( this->ConnectOptions ) > 0 )
    {
    options += this->ConnectOptions;
    }

  try
    {
    this->Connection = new vtkPostgreSQLDatabasePrivate( options.c_str() );
    this->ConnectionMTime.Modified();
    }
  catch ( pqxx::sql_error& e )
    {
    this->Connection->SetLastErrorText( e.what() );
    return false;
    }

  this->Connection->SetLastErrorText( NULL );
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
bool vtkPostgreSQLDatabase::HasError()
{
  // Assume that an unopened connection is not a symptom of failure.
  return this->Connection ? this->Connection->LastErrorText != NULL : false;
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
vtkStdString vtkPostgreSQLDatabase::GetURL()
{
  vtkStdString url = this->GetDatabaseType();
  url += "://";
  if ( this->HostName && this->DatabaseName )
    {
    if ( this->UserName )
      {
      url += this->UserName;
      if ( this->Password )
        {
        url += ":";
        url += this->Password;
        }
      url += "@";
      }
    url += this->HostName;
    url += "/";
    url += this->DatabaseName;
    }
  return url;
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
    this->Connection->SetLastErrorText( query->GetLastErrorText() );
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
  this->Connection->SetLastErrorText( 0 );
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
    this->Connection->SetLastErrorText( query->GetLastErrorText() );
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
  this->Connection->SetLastErrorText( 0 );
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
  if ( ! dbName )
    {
    vtkErrorMacro( "Databases must have a non-NULL name" );
    return false;
    }

  bool dropCurrentlyConnected = false;
  if ( this->DatabaseName && ! strcmp( this->DatabaseName, dbName ) )
    {
    dropCurrentlyConnected = true;
    if ( dropExisting )
      {
      // we can't drop a database we're connected to...
      this->SetDatabaseName( "template1" );
      this->Open();
      }
    else
      {
      // this will fail... let it. then report the error via LastErrorText
      }
    }

  if ( ! this->Connection )
    {
    bool err = true;
    if ( this->DatabaseName && this->HostName )
      {
      err = this->Open() ? false : true;
      }
    if ( err )
      {
      vtkErrorMacro( "Must be connected to a server to create a database." );
      return false;
      }
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

  if ( dropCurrentlyConnected )
    {
    this->SetDatabaseName( dbName );
    this->Open();
    }
  this->Connection->SetLastErrorText( 0 );
  return true;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::DropDatabase( const char* dbName )
{
  if ( ! dbName || strlen( dbName ) <= 0 )
    {
    vtkErrorMacro( "DropDatabase called with an empty database name" );
    return false;
    }

  if ( ! strcmp( dbName, this->DatabaseName ) )
    {
    // Can't drop database we're connected to... connect to the default db.
    this->SetDatabaseName( "template1" );
    }

  if ( ! this->Connection )
    {
    bool err = true;
    if ( this->DatabaseName && this->HostName )
      {
      err = this->Open() ? false : true;
      }
    if ( err )
      {
      vtkErrorMacro( "Must be connected to a server to create a database." );
      return false;
      }
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
    this->Connection->SetLastErrorText( e.what() );
    return false;
    }
  this->Connection->SetLastErrorText( 0 );
  return true;
}
