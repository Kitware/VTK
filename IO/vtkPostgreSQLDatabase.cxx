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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkPostgreSQLDatabase.h"
#include "vtkPostgreSQLDatabasePrivate.h"
#include "vtkPostgreSQLQuery.h"

#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#define PQXX_ALLOW_LONG_LONG
#include <pqxx/pqxx>

vtkStandardNewMacro(vtkPostgreSQLDatabase);
vtkCxxRevisionMacro(vtkPostgreSQLDatabase, "1.27");

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::vtkPostgreSQLDatabase()
{
  this->Connection = 0;
  this->ConnectionMTime = this->MTime;
  
  this->DatabaseType = 0;
  this->SetDatabaseType("psql");
  this->HostName = 0;
  this->User = 0;
  this->DatabaseName = 0;
  this->ServerPort = -1;
  this->ConnectOptions = 0;
  this->LastErrorText = 0;
}

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::~vtkPostgreSQLDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }

  this->SetHostName( 0 );
  this->SetUser( 0 );
  this->SetDatabaseName( 0 );
  this->SetConnectOptions( 0 );
  this->SetDatabaseType( 0 );
  this->SetLastErrorText( 0 );
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
  os << indent << "User: " << (this->User ? this->User : "NULL") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "NULL") << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "NULL") << endl;
  os << indent << "LastErrorText: " << this->LastErrorText << endl;
}

// ----------------------------------------------------------------------
vtkStdString vtkPostgreSQLDatabase::GetColumnSpecification(
  vtkSQLDatabaseSchema* schema, int tblHandle, int colHandle )
{
  vtksys_ios::ostringstream queryStr;
  queryStr << schema->GetColumnNameFromHandle( tblHandle, colHandle );

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle ); 
  vtkStdString colTypeStr;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    
      colTypeStr = "SERIAL";
      break;
    case vtkSQLDatabaseSchema::SMALLINT:  
      colTypeStr = "SMALLINT";
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colTypeStr = "INTEGER";
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colTypeStr = "BIGINT";
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colTypeStr = "VARCHAR";
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colTypeStr = "TEXT";
      break;
    case vtkSQLDatabaseSchema::REAL:      
      colTypeStr = "REAL";
      break;
    case vtkSQLDatabaseSchema::DOUBLE:    
      colTypeStr = "DOUBLE PRECISION";
      break;
    case vtkSQLDatabaseSchema::BLOB:      
      colTypeStr = "BYTEA";
      break;
    case vtkSQLDatabaseSchema::TIME:      
      colTypeStr = "TIME";
      break;
    case vtkSQLDatabaseSchema::DATE:      
      colTypeStr = "DATE";
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP: 
      colTypeStr = "TIMESTAMP WITH TIME ZONE";
      break;
    }
  
  if ( colTypeStr.size() )
    {
    queryStr << " " << colTypeStr;
    }
  else // if ( colTypeStr.size() )
    {
    vtkGenericWarningMacro( "Unable to get column specification: unsupported data type " << colType );
    return vtkStdString();
    }
  
  // Decide whether size is allowed, required, or unused
  int colSizeType = 0;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::SMALLINT:  
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colSizeType = -1;
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::REAL:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::DOUBLE:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::BLOB:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::TIME:      
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::DATE:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP: 
      colSizeType =  0;
      break;
    }

  // Specify size if allowed or required
  if ( colSizeType )
    {
    int colSize = schema->GetColumnSizeFromHandle( tblHandle, colHandle );
    // IF size is provided but absurd, 
    // OR, if size is required but not provided OR absurd,
    // THEN assign the default size.
    if ( ( colSize < 0 ) || ( colSizeType == -1 && colSize < 1 ) )
      {
      colSize = VTK_SQL_DEFAULT_COLUMN_SIZE;
      }
    
    // At this point, we have either a valid size if required, or a possibly null valid size
    // if not required. Thus, skip sizing in the latter case.
    if ( colSize > 0 )
      {
      queryStr << "(" << colSize << ")";
      }
    }

  vtkStdString attStr = schema->GetColumnAttributesFromHandle( tblHandle, colHandle );
  if ( attStr.size() )
    {
    queryStr << " " << attStr;
    }

  return queryStr.str();
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLDatabase::Open(const char* password)
{
  if ( ! this->HostName || ! this->DatabaseName )
    {
    this->SetLastErrorText( "Cannot open database because HostName and/or DatabaseName are null." );
    vtkErrorMacro( << this->GetLastErrorText() );
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

  vtkstd::string options;
  options = "dbname=";
  options += this->DatabaseName;

  if ( this->ServerPort )
    {
    options += " port=";
    vtksys_ios::ostringstream stream;
    stream << this->ServerPort;
    options += stream.str();
    }
  if ( this->User && strlen( this->User ) > 0 )
    {
    options += " user=";
    options += this->User;
    }
  if ( password && strlen( password ) > 0 )
    {
    options += " password=";
    options += password;
    }
  if ( this->ConnectOptions && strlen( this->ConnectOptions ) > 0 )
    {
    options += this->ConnectOptions;
    }

  // If localhost is specified, try the local socket connection
  // first. Only if that doesn't work will we try the loopback
  // device.
  if ( ! strcmp( this->HostName, "localhost" ) )
    {
    if ( this->OpenInternal( options.c_str() ) )
      {
      this->SetLastErrorText( 0 );
      this->Connection->LastErrorText.clear();
      this->Password = password ? password : "";
      return true;
      }
    }
  vtkstd::string hspec( "host=" );
  hspec += this->HostName;
  options = hspec + " " + options;
  if ( this->OpenInternal( options.c_str() ) )
    {
    this->SetLastErrorText( 0 );
    this->Connection->LastErrorText.clear();
    this->Password = password ? password : "";
    return true;
    }

  return false;
}

// ----------------------------------------------------------------------
void vtkPostgreSQLDatabase::Close()
{
  if ( this->Connection )
    {
    delete this->Connection;
    this->Connection = 0;
    this->SetLastErrorText( 0 );
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
  if ( this->Connection )
    return this->Connection->LastErrorText.empty() ? false : true;
  return this->LastErrorText ? true : false;
}

// ----------------------------------------------------------------------
const char* vtkPostgreSQLDatabase::GetLastErrorText()
{
  return this->Connection ?
    this->Connection->LastErrorText.c_str() : this->LastErrorText;
}

// ----------------------------------------------------------------------
vtkStdString vtkPostgreSQLDatabase::GetURL()
{
  vtkStdString url = this->GetDatabaseType();
  url += "://";
  if ( this->HostName && this->DatabaseName )
    {
    if ( this->User )
      {
      url += this->User;
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
    vtkErrorMacro(<< "Database returned error: " << query->GetLastErrorText());
    this->Connection->LastErrorText = query->GetLastErrorText();
    query->Delete();
    return 0;
    }
  vtkDebugMacro(<< "GetTables(): SQL query succeeded.");
  vtkStringArray* results = vtkStringArray::New();
  while ( query->NextRow() )
    {
    results->InsertNextValue( query->DataValue( 0 ).ToString() );
    }
  query->Delete();
  this->Connection->LastErrorText.clear();
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
    this->Connection->LastErrorText = query->GetLastErrorText();
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
  this->Connection->LastErrorText.clear();
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
    case VTK_SQL_FEATURE_TRIGGERS:
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

  vtkstd::string qstr( "CREATE DATABASE \"" );
  qstr += dbName;
  qstr += "\"";
  try
    {
    pqxx::nontransaction work( this->Connection->Connection );
    pqxx::result res = work.exec( qstr.c_str() );
    work.commit();
    }
  catch ( const vtkstd::exception& e )
    {
    vtkErrorMacro(
      "Could not create database \"" << dbName << "\". "
      << this->GetLastErrorText() << "\n" );
    return false;
    }

  if ( dropCurrentlyConnected )
    {
    this->SetDatabaseName( dbName );
    this->Open();
    }
  if ( this->Connection )
    {
    this->Connection->LastErrorText.clear();
    }
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

  vtkstd::string qstr( "DROP DATABASE \"" );
  qstr += dbName;
  qstr += "\"";
  //qstr += " IF EXISTS";
  try
    {
    pqxx::nontransaction work( this->Connection->Connection );
    pqxx::result res = work.exec( qstr.c_str() );
    work.commit();
    }
  catch ( const vtkstd::exception& e )
    {
    this->Connection->LastErrorText = e.what();
    return false;
    }
  if ( this->Connection )
    {
    this->Connection->LastErrorText.clear();
    }
  return true;
}

void vtkPostgreSQLDatabase::NullTrailingWhitespace( char* msg )
{
  // overwrite any blank space at the end of a message with NULL.
  // PostgreSQL error messages are terminated with a newline, which
  // does not work well with VTK's already lengthy error output.
  int msglen = strlen( msg );
  char* tail = msg + msglen - 1;
  while ( tail > msg && isspace( *tail ) )
    *(tail--) = 0;
}

bool vtkPostgreSQLDatabase::OpenInternal( const char* connectionOptions )
{
  try
    {
    this->Connection = new vtkPostgreSQLDatabasePrivate( connectionOptions );
    this->ConnectionMTime.Modified();
    }
  catch ( pqxx::sql_error& e )
    {
    this->SetLastErrorText( e.what() );
    this->NullTrailingWhitespace( this->LastErrorText );
    this->Connection = 0; // we weren't able to construct
    return false;
    }
  catch ( pqxx::broken_connection& e )
    {
    this->SetLastErrorText( e.what() );
    this->NullTrailingWhitespace( this->LastErrorText );
    this->Connection = 0; // we weren't able to construct
    return false;
    }
  return true;
}

