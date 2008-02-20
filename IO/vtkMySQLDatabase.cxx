/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMySQLDatabase.cxx

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
#include "vtkMySQLDatabase.h"
#include "vtkMySQLQuery.h"

#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#include <assert.h>
#include <mysql.h>

#define VTK_MYSQL_DEFAULT_PORT 3306
 
vtkCxxRevisionMacro(vtkMySQLDatabase, "1.11");
vtkStandardNewMacro(vtkMySQLDatabase);

// ----------------------------------------------------------------------
vtkMySQLDatabase::vtkMySQLDatabase()
{
  this->Connection = NULL;

  mysql_init(& this->NullConnection);
  this->Tables = vtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();
  
  // Initialize instance variables
  this->DatabaseType = 0;
  this->SetDatabaseType( "mysql" );
  this->HostName = 0;
  this->UserName = 0;
  this->Password = 0;
  this->DatabaseName = 0;
  this->ConnectOptions = 0;
  // Default: connect to local machine on standard port
  this->SetHostName( "localhost" );
  this->ServerPort = VTK_MYSQL_DEFAULT_PORT;
  //this->SetPassword( "" );
}

// ----------------------------------------------------------------------
vtkMySQLDatabase::~vtkMySQLDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }
  this->SetDatabaseType( 0 );
  this->SetHostName( 0 );
  this->SetUserName( 0 );
  this->SetPassword( 0 );
  this->SetDatabaseName( 0 );
  this->SetConnectOptions( 0 );

  this->Tables->UnRegister(this);
}

// ----------------------------------------------------------------------
bool vtkMySQLDatabase::IsSupported(int feature)
{
  switch (feature)
    {
    case VTK_SQL_FEATURE_BATCH_OPERATIONS:
    case VTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
      return false;

    case VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
#if MYSQL_VERSION_ID >= 40108
      return true;
#else
      return false;
#endif

    case VTK_SQL_FEATURE_PREPARED_QUERIES:
    {
    if (mysql_get_client_version() >= 40108 &&
        mysql_get_server_version(& this->NullConnection) >= 40100)
      {
      return true;
      }
    else
      {
      return false;
      }
    };

    case VTK_SQL_FEATURE_QUERY_SIZE:
    case VTK_SQL_FEATURE_BLOB:
    case VTK_SQL_FEATURE_LAST_INSERT_ID:
    case VTK_SQL_FEATURE_UNICODE:
    case VTK_SQL_FEATURE_TRANSACTIONS:
      return true;
      
    default:
    {
    vtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                  << "vtkSQLDatabase.h for a list of possible features.");
    return false;
    };
    }
}

// ----------------------------------------------------------------------
bool vtkMySQLDatabase::Open()
{

  if ( this->IsOpen() )
    {
    vtkGenericWarningMacro( "Open(): Database is already open." );
    return true;
    }

  assert(this->Connection == NULL);

  this->Connection = 
    mysql_real_connect( &this->NullConnection, 
                        this->GetHostName(),
                        this->GetUserName(),
                        this->GetPassword(), 
                        this->GetDatabaseName(),
                        this->GetServerPort(),
                        0, 0);
                                        
  if (this->Connection == NULL)
    {
    vtkErrorMacro(<<"Open() failed with error: " 
                  << mysql_error(& this->NullConnection));
    return false;
    }
  else
    {
    vtkDebugMacro(<<"Open() succeeded.");
    return true;
    }
}

// ----------------------------------------------------------------------
void vtkMySQLDatabase::Close()
{
  if (! this->IsOpen())
    {
    return; // not an error
    }
  else
    {
    mysql_close(this->Connection);
    this->Connection = NULL;
    }
}

// ----------------------------------------------------------------------
bool vtkMySQLDatabase::IsOpen()
{
  return (this->Connection != NULL);
}

// ----------------------------------------------------------------------
vtkSQLQuery* vtkMySQLDatabase::GetQueryInstance()
{
  vtkMySQLQuery* query = vtkMySQLQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------
vtkStringArray* vtkMySQLDatabase::GetTables()
{
  this->Tables->Resize(0);
  if ( ! this->IsOpen() )
    {
    vtkErrorMacro(<<"GetTables(): Database is closed!");
    return this->Tables;
    }
  else
    {
    MYSQL_RES* tableResult = mysql_list_tables(
      this->Connection, NULL );

    if ( ! tableResult )
      {
      vtkErrorMacro(<<"GetTables(): MySQL returned error: "
                    << mysql_error(this->Connection));
      return this->Tables;
      }

    MYSQL_ROW row;
    int i=0;

    while ( tableResult )
      {
      mysql_data_seek( tableResult, i );
      row = mysql_fetch_row( tableResult );
      if ( ! row )
        {
        break;
        }

      this->Tables->InsertNextValue( row[0] );
      ++ i;
      }
      // Done with processing so free it
      mysql_free_result( tableResult );

    return this->Tables;
    }
}

// ----------------------------------------------------------------------
vtkStringArray* vtkMySQLDatabase::GetRecord(const char *table)
{
  vtkStringArray *results = vtkStringArray::New();

  if (!this->IsOpen())
    {
    vtkErrorMacro(<<"GetRecord: Database is not open!");
    return results;
    }

  MYSQL_RES *record = 
    mysql_list_fields(this->Connection, table, 0);

  if (!record)
    {
    vtkErrorMacro(<<"GetRecord: MySQL returned error: "
                  << mysql_error(this->Connection));
    return results;
    }

  MYSQL_FIELD *field;
  while ((field = mysql_fetch_field(record)))
    {
    results->InsertNextValue(field->name);
    }
  
  mysql_free_result(record);
  return results;
}


bool vtkMySQLDatabase::HasError()
{ 
  return (mysql_errno(this->Connection)!=0);
}

const char* vtkMySQLDatabase::GetLastErrorText()
{
  return mysql_error(this->Connection);
}

// ----------------------------------------------------------------------
vtkStdString vtkMySQLDatabase::GetURL()
{
  vtkStdString url;
  url = this->GetDatabaseType();
  url += "://";
  if ( this->GetUserName() && strlen( this->GetUserName() ) )
    {
    url += this->GetUserName();
    if ( this->GetPassword() && strlen( this->GetPassword() ) )
      {
      url += ":";
      url += this->GetPassword();
      }
    url += "@";
    }
  if ( this->GetHostName() && strlen( this->GetHostName() ) )
    {
    url += this->GetHostName();
    }
  else
    {
    url += "localhost";
    }
  if (
    this->GetServerPort() >= 0 &&
    this->GetServerPort() != VTK_MYSQL_DEFAULT_PORT
    )
    {
    url += ":";
    url += this->GetServerPort();
    }
  url += "/";
  url += this->GetDatabaseName();
  return url;
}

// ----------------------------------------------------------------------
vtkStdString vtkMySQLDatabase::GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                                       int tblHandle,
                                                       int colHandle )
{
  // With MySQL, the column name must be enclosed between backquotes
  vtkStdString queryStr = "`";
  queryStr += schema->GetColumnNameFromHandle( tblHandle, colHandle );
  queryStr += "` ";

  int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle ); 

  vtkStdString colTypeStr = this->GetColumnTypeString( colType );
  if ( colTypeStr )
    {
    queryStr += " ";
    queryStr += colTypeStr;
    }
  else // if ( colTypeStr )
    {
    vtkGenericWarningMacro( "Unable to get column specification: unsupported data type " << colType );
    return 0;
    }
  
  vtkStdString attStr = schema->GetColumnAttributesFromHandle( tblHandle, colHandle );
  if ( attStr )
    {
    queryStr += " ";
    queryStr += attStr;
    }

  return queryStr;
}

// ----------------------------------------------------------------------
vtkStdString vtkMySQLDatabase::GetColumnTypeString( int colType )
{
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL: return "INT NOT NULL AUTO_INCREMENT";
    case vtkSQLDatabaseSchema::SMALLINT: return "SMALLINT";
    case vtkSQLDatabaseSchema::INTEGER: return "INT";
    case vtkSQLDatabaseSchema::BIGINT: return "BIGINT";
    case vtkSQLDatabaseSchema::VARCHAR: return "VARCHAR";
    case vtkSQLDatabaseSchema::TEXT: return "TEXT";
    case vtkSQLDatabaseSchema::REAL: return "DOUBLE";
    case vtkSQLDatabaseSchema::DOUBLE: return "DOUBLE PRECISION";
    case vtkSQLDatabaseSchema::BLOB: return "BLOB";
    case vtkSQLDatabaseSchema::TIME: return "TIME";
    case vtkSQLDatabaseSchema::DATE: return "DATE";
    case vtkSQLDatabaseSchema::TIMESTAMP: return "TIMESTAMP";
    }

    return 0;
}

// ----------------------------------------------------------------------
void vtkMySQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
  os << indent << "HostName: " << (this->HostName ? this->HostName : "NULL") << endl;
  os << indent << "UserName: " << (this->UserName ? this->UserName : "NULL") << endl;
  os << indent << "Password: " << (this->Password ? this->Password : "NULL") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "NULL") << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "NULL") << endl;
}
