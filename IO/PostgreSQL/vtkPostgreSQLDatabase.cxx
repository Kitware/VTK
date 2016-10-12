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

#include <sstream>
#include <vtksys/SystemTools.hxx>

#include "vtkSmartPointer.h"
#define VTK_CREATE(classname, varname) vtkSmartPointer<classname> varname = vtkSmartPointer<classname>::New()

#include <libpq-fe.h>

#include <cassert>

vtkStandardNewMacro(vtkPostgreSQLDatabase);

// ----------------------------------------------------------------------
vtkPostgreSQLDatabase::vtkPostgreSQLDatabase()
{
  this->Connection = 0;
  this->ConnectionMTime = this->MTime;

  this->DatabaseType = 0;
  this->SetDatabaseType("psql");
  this->HostName = 0;
  this->User = 0;
  this->Password = 0;
  this->DatabaseName = 0;
  this->ServerPort = -1;
  this->ConnectOptions = 0;
  this->LastErrorText = 0;
  this->Tables = vtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();
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
  this->Tables->UnRegister(this);
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
  os << indent << "Password: " << (this->Password? "(hidden)":"(none)") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "NULL") << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "NULL") << endl;
  os << indent << "LastErrorText: " << this->LastErrorText << endl;
}

// ----------------------------------------------------------------------
vtkStdString vtkPostgreSQLDatabase::GetColumnSpecification(
  vtkSQLDatabaseSchema* schema, int tblHandle, int colHandle )
{
  std::ostringstream queryStr;
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
bool vtkPostgreSQLDatabase::Open( const char* password )
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

  std::string options;
  options = "dbname=";
  options += this->DatabaseName;

  if ( this->ServerPort > 0 )
  {
    options += " port=";
    std::ostringstream stream;
    stream << this->ServerPort;
    options += stream.str();
  }
  if ( this->User && strlen( this->User ) > 0 )
  {
    options += " user=";
    options += this->User;
  }
  if ( password && this->Password != password )
  {
    delete [] this->Password;
    this->Password = password ? vtksys::SystemTools::DuplicateString( password ) : 0;
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

  // If localhost is specified, try the local socket connection
  // first. Only if that doesn't work will we try the loopback
  // device.
  if ( ! strcmp( this->HostName, "localhost" ) )
  {
    if ( this->OpenInternal( options.c_str() ) )
    {
      this->SetLastErrorText( 0 );
      return true;
    }
  }
  std::string hspec( "host=" );
  hspec += this->HostName;
  options = hspec + " " + options;
  if ( this->OpenInternal( options.c_str() ) )
  {
    this->SetLastErrorText( 0 );
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
  return (this->Connection != 0 &&
          this->Connection->Connection != 0 &&
          PQstatus(this->Connection->Connection) == CONNECTION_OK);
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
  {
    return this->LastErrorText ? true : false;
  }
  else
  {
    return false;
  }
}

// ----------------------------------------------------------------------
const char* vtkPostgreSQLDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------
vtkStdString vtkPostgreSQLDatabase::GetURL()
{
  vtkStdString url = this->GetDatabaseType();
  url += "://";
  if ( this->HostName && this->DatabaseName )
  {
    if ( this->User && strlen( this->User ) > 0 )
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
bool vtkPostgreSQLDatabase::ParseURL( const char* URL )
{
  std::string urlstr( URL ? URL : "" );
  std::string protocol;
  std::string username;
  std::string password;
  std::string hostname;
  std::string dataport;
  std::string database;

  // Okay now for all the other database types get more detailed info
  if ( ! vtksys::SystemTools::ParseURL(
      urlstr, protocol, username, password, hostname, dataport, database) )
  {
    vtkErrorMacro( "Invalid URL: \"" << urlstr.c_str() << "\"" );
    return false;
  }

  if ( protocol == "psql" )
  {
    this->SetUser( username.empty() ? 0 : username.c_str() );
    this->SetPassword( password.empty() ? 0 : password.c_str() );
    this->SetHostName( hostname.empty() ? 0 : hostname.c_str() );
    this->SetServerPort( atoi( dataport.c_str() ) );
    this->SetDatabaseName( database.empty() ? 0 : database.c_str() );
    return true;
  }

  return false;
}

// ----------------------------------------------------------------------
vtkStringArray* vtkPostgreSQLDatabase::GetTables()
{
  this->Tables->Resize(0);
  if ( ! this->Connection )
  {
    vtkErrorMacro(<< this->GetLastErrorText());
    return this->Tables;
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
    this->SetLastErrorText(query->GetLastErrorText());
    query->Delete();
    return this->Tables;
  }
  vtkDebugMacro(<< "GetTables(): SQL query succeeded.");
  while ( query->NextRow() )
  {
    this->Tables->InsertNextValue( query->DataValue( 0 ).ToString() );
  }
  query->Delete();
  this->SetLastErrorText(NULL);
  return this->Tables;
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
    this->SetLastErrorText(query->GetLastErrorText());
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
  this->SetLastErrorText(0);
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
    if ( this->DatabaseName && ! strcmp( this->DatabaseName, dbName ) )
    {
      // we can't connect to a database we haven't created yet and aren't connected to...
      this->SetDatabaseName( "template1" );
      dropCurrentlyConnected = true;
    }
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

  std::string qstr( "CREATE DATABASE \"" );
  qstr += dbName;
  qstr += "\"";
  vtkSQLQuery *query = this->GetQueryInstance();
  query->SetQuery(qstr.c_str());
  if (query->Execute() == false)
  {
    this->SetLastErrorText(query->GetLastErrorText());
    vtkErrorMacro(
      "Could not create database \"" << dbName << "\". "
      << this->GetLastErrorText() << "\n");
    query->Delete();
    return false;
  }

  query->Delete();
  this->SetLastErrorText(0);
  if ( dropCurrentlyConnected )
  {
    this->SetDatabaseName( dbName );
    this->Open();
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

  std::string qstr( "DROP DATABASE IF EXISTS \"" );
  qstr += dbName;
  qstr += "\"";
  vtkSQLQuery *query = this->GetQueryInstance();
  query->SetQuery(qstr.c_str());
  if (query->Execute() == false)
  {
    this->SetLastErrorText(query->GetLastErrorText());
    vtkErrorMacro(<<"Could not drop database \""
                  << dbName << "\".  "
                  << query->GetLastErrorText());
    query->Delete();
    return false;
  }
  this->SetLastErrorText(0);
  query->Delete();
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
  assert(this->Connection == NULL);
  this->Connection = new vtkPostgreSQLDatabasePrivate;
  this->Connection->Connection = PQconnectdb(connectionOptions);
  if (PQstatus(this->Connection->Connection) == CONNECTION_OK)
  {
    this->SetLastErrorText(0);
    this->UpdateDataTypeMap();
    return true;
  }
  else
  {
    this->SetLastErrorText(PQerrorMessage(this->Connection->Connection));
    vtkErrorMacro(<<"Unable to open database connection. "
                  << this->GetLastErrorText());
    delete this->Connection;
    this->Connection = 0;
    return false;
  }
}

// ----------------------------------------------------------------------

void vtkPostgreSQLDatabase::UpdateDataTypeMap()
{
  if (!this->IsOpen())
  {
    return;
  }

  this->Connection->DataTypeMap.clear();

  vtkSQLQuery *typeQuery = this->GetQueryInstance();
  typeQuery->SetQuery("SELECT oid, typname, typlen FROM pg_type");
  bool status = typeQuery->Execute();
  if (!status)
  {
    vtkErrorMacro(<<"I was totally surprised to see the data type query fail.  Error message: "
                  << typeQuery->GetLastErrorText());
    typeQuery->Delete();
  }
  else
  {
    while (typeQuery->NextRow())
    {
      Oid oid;
      vtkStdString name;
      int len;

      // Caution: this assumes that the Postgres OID type is a 32-bit
      // unsigned int.
      oid = typeQuery->DataValue(0).ToUnsignedInt();
      name = typeQuery->DataValue(1).ToString();
      len = typeQuery->DataValue(2).ToInt();

      if ( name == "int8" || ( name == "oid" && len == 8 ) )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_TYPE_INT64;
      }
      else if ( name == "int4" || ( name == "oid" && len == 4 ) )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_TYPE_INT32;
      }
      else if ( name == "int2" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_TYPE_INT16;
      }
      else if ( name == "char" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_TYPE_INT8;
      }
      else if ( name == "time_stamp" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_TYPE_INT64;
      }
      else if ( name == "float4" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_FLOAT;
      }
      else if ( name == "float8" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_DOUBLE;
      }
      else if ( name == "abstime" || name == "reltime" )
      {
        this->Connection->DataTypeMap[ oid ] = ( len == 4 ? VTK_TYPE_INT32 : VTK_TYPE_INT64 );
      }
      else if ( name == "text" )
      {
        this->Connection->DataTypeMap[ oid ] = VTK_STRING;
      }
    } // done looping over rows
  } // done with "query is successful"
  typeQuery->Delete();
}

