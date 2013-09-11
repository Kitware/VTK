/*=========================================================================

  Program Toolkit
  Module:    vtkODBCDatabase.cxx

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
  ----------------------------------------------------------------------------
*/

/*
 * Microsoft's own version of sqltypes.h relies on some typedefs and
 * macros in windows.h.  This next fragment tells VTK to include the
 * whole thing without any of its usual #defines to keep the size
 * manageable.  No WIN32_LEAN_AND_MEAN for us!
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
# include <vtkWindows.h>
#endif


#include <vtkSQLDatabaseSchema.h>

#include "vtkODBCDatabase.h"
#include "vtkODBCQuery.h"
#include "vtkODBCInternals.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#include <cassert>
#include <string.h>

#include <sql.h>
#include <sqlext.h>


// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkODBCDatabase);

// ----------------------------------------------------------------------------
static vtkStdString GetErrorMessage(SQLSMALLINT handleType,
                                    SQLHANDLE handle,
                                    int *code=0)
{
  SQLINTEGER sqlNativeCode = 0;
  SQLSMALLINT messageLength = 0;
  SQLRETURN status;
  SQLCHAR state[SQL_SQLSTATE_SIZE + 1];
  SQLCHAR description[SQL_MAX_MESSAGE_LENGTH + 1];
  vtkStdString finalResult;
  int i = 1;

  // There may be several error messages queued up so we need to loop
  // until we've got everything.
  vtksys_ios::ostringstream messagebuf;
  do
    {
    status = SQLGetDiagRec(handleType, handle,
                           i,
                           state,
                           &sqlNativeCode,
                           description,
                           SQL_MAX_MESSAGE_LENGTH,
                           &messageLength);

    description[SQL_MAX_MESSAGE_LENGTH] = 0;
    if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
      {
      if (code)
        {
        *code = sqlNativeCode;
        }
      if (i > 1)
        {
        messagebuf << ", ";
        }
      messagebuf << state << ' ' << description;
      }
    else if (status == SQL_ERROR || status == SQL_INVALID_HANDLE)
      {
      return vtkStdString(messagebuf.str());
      }
    ++i;
    } while (status != SQL_NO_DATA);

  return vtkStdString(messagebuf.str());
}

// ----------------------------------------------------------------------------
// COLUMN is zero-indexed but ODBC indexes from 1.  Sigh.  Aren't
// standards fun?
//
// Also, this will need to be updated when we start handling Unicode
// characters.

static vtkStdString odbcGetString(SQLHANDLE statement,
                                  int column,
                                  int columnSize)
{
  vtkStdString returnString;
  SQLRETURN status = SQL_ERROR;
  SQLLEN lengthIndicator;

  // Make sure we've got room to store the results but don't go past 64k
  if (columnSize <= 0)
    {
    columnSize = 1024;
    }
  else if (columnSize > 65536)
    {
    columnSize = 65536;
    }
  else
    {
    // make room for the null terminator
    ++ columnSize;
    }

  char *buffer = new char[columnSize];
  while (true)
    {
    status = SQLGetData(statement,
                        column+1,
                        SQL_C_CHAR,
                        static_cast<SQLPOINTER>(buffer),
                        columnSize,
                        &lengthIndicator);
    if (status == SQL_SUCCESS ||
        status == SQL_SUCCESS_WITH_INFO)
      {
      if (lengthIndicator == SQL_NULL_DATA ||
          lengthIndicator == SQL_NO_TOTAL)
        {
        break;
        }
      int resultSize = 0;
      if (status == SQL_SUCCESS_WITH_INFO)
        {
        // SQL_SUCCESS_WITH_INFO means that there's more data to
        // retrieve so we have to do it in chunks -- hence the while
        // loop.
        resultSize = columnSize - 1;
        }
      else
        {
        resultSize = lengthIndicator;
        }
      buffer[resultSize] = 0;
      returnString += buffer;
      }
    else if (status == SQL_NO_DATA)
      {
      // we're done
      break;
      }
    else
      {
      cerr << "odbcGetString: Error "
           << status << " in SQLGetData\n";

      break;
      }
    }

  delete [] buffer;
  return returnString;
}

// ----------------------------------------------------------------------------
vtkODBCDatabase::vtkODBCDatabase()
{
  this->Internals = new vtkODBCInternals;

  this->Tables = vtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();
  this->LastErrorText = NULL;

  this->Record = vtkStringArray::New();
  this->Record->Register(this);
  this->Record->Delete();

  this->UserName = NULL;
  this->HostName = NULL;
  this->DataSourceName = NULL;
  this->DatabaseName = NULL;
  this->Password = NULL;

  this->ServerPort = -1; // use whatever the driver defaults to

  // Initialize instance variables
  this->DatabaseType = 0;
  this->SetDatabaseType("ODBC");
}

// ----------------------------------------------------------------------------
vtkODBCDatabase::~vtkODBCDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }
  if ( this->DatabaseType )
    {
    this->SetDatabaseType(0);
    }
  this->SetLastErrorText(NULL);
  this->SetUserName(NULL);
  this->SetHostName(NULL);
  this->SetPassword(NULL);
  this->SetDataSourceName(NULL);
  this->SetDatabaseName(NULL);
  delete this->Internals;

  this->Tables->UnRegister(this);
  this->Record->UnRegister(this);
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::IsSupported(int feature)
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
    return true;
    }

    case VTK_SQL_FEATURE_UNICODE:
      return false; // not until we have vtkStdWideString

    case VTK_SQL_FEATURE_QUERY_SIZE:
    case VTK_SQL_FEATURE_BLOB:
    case VTK_SQL_FEATURE_LAST_INSERT_ID:
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

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::Open(const char *password)
{
  if  ( ! this->DataSourceName )
    {
    this->SetLastErrorText("Cannot open database because database ID is null.");
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
    }

  if ( this->IsOpen() )
    {
    vtkGenericWarningMacro( "Open(): Database is already open." );
    return true;
    }

  SQLRETURN status;
  status = SQLAllocHandle(SQL_HANDLE_ENV,
                          SQL_NULL_HANDLE,
                          & (this->Internals->Environment));

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    // We don't actually have a valid SQL handle yet so I don't think
    // we can actually retrieve an error message.
    vtksys_ios::ostringstream sbuf;
    sbuf << "vtkODBCDatabase::Open: Unable to allocate environment handle.  "
         << "Return code " << status << ", error message: "
         << GetErrorMessage(SQL_HANDLE_ENV,
                            this->Internals->Environment);
    this->SetLastErrorText(sbuf.str().c_str());
    return false;
    }
  else
    {
    vtkDebugMacro(<<"Successfully allocated environment handle.");
    status = SQLSetEnvAttr(this->Internals->Environment,
                           SQL_ATTR_ODBC_VERSION,
                           reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3),
                           SQL_IS_UINTEGER);
    }

  // Create the connection string itself
  vtkStdString connectionString;
  if (strstr(this->DataSourceName, ".dsn") != NULL)
    {
    // the data source is a file of some sort
    connectionString = "FILEDSN=";
    connectionString += this->DataSourceName;
    }
  else if (strstr(this->DataSourceName, "DRIVER") != NULL ||
           strstr(this->DataSourceName, "SERVER"))
    {
    connectionString = this->DataSourceName;
    }
  else
    {
    connectionString = "DSN=";
    connectionString += this->DataSourceName;
    }

  if (this->UserName != NULL && strlen(this->UserName) > 0)
    {
    connectionString += ";UID=";
    connectionString += this->UserName;
    }
  if (password != NULL)
    {
    connectionString += ";PWD=";
    connectionString += password;
    }
  if (this->DatabaseName != NULL && strlen(this->DatabaseName) > 0)
    {
    connectionString += ";DATABASE=";
    connectionString += this->DatabaseName;
    }

  // Get a handle to connect with
  status = SQLAllocHandle(SQL_HANDLE_DBC,
                          this->Internals->Environment,
                          &(this->Internals->Connection));

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "Error allocating ODBC connection handle: "
           << GetErrorMessage(SQL_HANDLE_ENV, this->Internals->Environment);
    this->SetLastErrorText(errbuf.str().c_str());
    return false;
    }

  vtkDebugMacro(<<"ODBC connection handle successfully allocated");


#ifdef ODBC_DRIVER_IS_IODBC
  // Set the driver name so we know who to blame
  vtkStdString driverName("vtkODBCDatabase driver");
  status = SQLSetConnectAttr(this->Internals->Connection,
                             SQL_APPLICATION_NAME,
                             driverName.c_str(),
                             driverName.size());
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "Error setting driver name: "
           << GetErrorMessage(SQL_HANDLE_DBC, this->Internals->Connection);
    this->SetLastErrorText(errbuf.str().c_str());
    return false;
    }
  else
    {
    vtkDebugMacro(<<"Successfully set driver name on connect string.");
    }
#endif

  SQLSMALLINT cb;
  SQLTCHAR connectionOut[1024];
  status = SQLDriverConnect(this->Internals->Connection,
                            NULL,
                            (SQLCHAR *)(connectionString.c_str()),
                            connectionString.size(),
                            connectionOut,
                            1024,
                            &cb,
                            SQL_DRIVER_NOPROMPT);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream sbuf;
    sbuf << "vtkODBCDatabase::Open: Error during connection: "
         << GetErrorMessage(SQL_HANDLE_DBC, this->Internals->Connection);
    this->SetLastErrorText(sbuf.str().c_str());
    return false;
    }

  vtkDebugMacro(<<"Connection successful.");

  return true;
}

// ----------------------------------------------------------------------------
void vtkODBCDatabase::Close()
{
  if (! this->IsOpen())
    {
    return; // not an error
    }
  else
    {
    SQLRETURN status;

    if (this->Internals->Connection != SQL_NULL_HDBC)
      {
      status = SQLDisconnect(this->Internals->Connection);
      if (status != SQL_SUCCESS)
        {
        vtkWarningMacro(<< "ODBC Close: Unable to disconnect data source");
        }
      status = SQLFreeHandle(SQL_HANDLE_DBC, this->Internals->Connection);
      if (status != SQL_SUCCESS)
        {
        vtkWarningMacro(<< "ODBC Close: Unable to free connection handle");
        }
      this->Internals->Connection = NULL;
      }

    if (this->Internals->Environment != SQL_NULL_HENV)
      {
      status = SQLFreeHandle(SQL_HANDLE_ENV, this->Internals->Environment);
      if (status != SQL_SUCCESS)
        {
        vtkWarningMacro(<< "ODBC Close: Unable to free environment handle");
        }
      this->Internals->Environment = NULL;
      }
    }
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::IsOpen()
{
  return (this->Internals->Connection != SQL_NULL_HDBC);
}

// ----------------------------------------------------------------------------
vtkSQLQuery *vtkODBCDatabase::GetQueryInstance()
{
  vtkODBCQuery *query = vtkODBCQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------------
const char*vtkODBCDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------------
vtkStringArray *vtkODBCDatabase::GetTables()
{
  this->Tables->Resize(0);
  if (!this->IsOpen())
    {
    vtkErrorMacro(<<"GetTables(): Database is closed!");
    return this->Tables;
    }
  else
    {
    SQLHANDLE statement;
    SQLRETURN status = SQLAllocHandle(SQL_HANDLE_STMT,
                                      this->Internals->Connection,
                                      &statement);

    if (status != SQL_SUCCESS)
      {
      vtkErrorMacro(<< "vtkODBCDatabase::GetTables: Unable to allocate statement");

      return this->Tables;
      }

    status = SQLSetStmtAttr(statement,
                            SQL_ATTR_CURSOR_TYPE,
                            static_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY),
                            SQL_IS_UINTEGER);

    vtkStdString tableType("TABLE,");

    status = SQLTables(statement, NULL, 0, NULL, 0, NULL, 0,
                       (SQLCHAR *)(tableType.c_str()),
                       tableType.size());

    if (status != SQL_SUCCESS)
      {
      vtkErrorMacro(<< "vtkODBCDatabase::GetTables: Unable to execute table list");
      return this->Tables;
      }

    status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
    while (status == SQL_SUCCESS)
      {
      vtkStdString fieldVal = odbcGetString(statement, 2, -1);
      this->Tables->InsertNextValue(fieldVal);
      status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
      }

    status = SQLFreeHandle(SQL_HANDLE_STMT, statement);
    if (status != SQL_SUCCESS)
      {
      vtkErrorMacro(<<"vtkODBCDatabase::GetTables: Unable to free statement handle.  Error "
                    << status);
      }
    return this->Tables;
    }
}

// ----------------------------------------------------------------------------
vtkStringArray *vtkODBCDatabase::GetRecord(const char *table)
{
  this->Record->Reset();
  this->Record->Allocate(20);

  if (!this->IsOpen())
    {
    vtkErrorMacro(<<"GetRecord: Database is not open!");
    return this->Record;
    }

  SQLHANDLE statement;
  SQLRETURN status = SQLAllocHandle(SQL_HANDLE_STMT,
                                    this->Internals->Connection,
                                    &statement);
  if (status != SQL_SUCCESS)
    {
    vtkErrorMacro(<<"vtkODBCDatabase: Unable to allocate statement: error "
                  << status);
    return this->Record;
    }

  status = SQLSetStmtAttr(statement,
                          SQL_ATTR_METADATA_ID,
                          reinterpret_cast<SQLPOINTER>(SQL_TRUE),
                          SQL_IS_INTEGER);

  if (status != SQL_SUCCESS)
    {
    vtkErrorMacro(<<"vtkODBCDatabase::GetRecord: Unable to set SQL_ATTR_METADATA_ID attribute on query.  Return code: " << status);
    return NULL;
    }

  status = SQLSetStmtAttr(statement,
                          SQL_ATTR_CURSOR_TYPE,
                          static_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY),
                          SQL_IS_UINTEGER);

  status = SQLColumns(statement,
                      NULL, // catalog
                      0,
                      NULL, // schema
                      0,
                      (SQLCHAR *)(table),
                      strlen(table),
                      NULL, // column
                      0);

  if (status != SQL_SUCCESS && status != 0)
    {
    vtkStdString error = GetErrorMessage(SQL_HANDLE_STMT, statement);

    vtkErrorMacro(<<"vtkODBCDatabase::GetRecord: Unable to retrieve column list (SQLColumns): error " << error.c_str());
    this->SetLastErrorText(error.c_str());
    SQLFreeHandle(SQL_HANDLE_STMT, statement);
    return this->Record;

    }

  status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
  if (status != SQL_SUCCESS)
    {
    vtkStdString error = GetErrorMessage(SQL_HANDLE_STMT, statement);
    vtkErrorMacro(<<"vtkODBCDatabase::GetRecord: Unable to retrieve column list (SQLFetchScroll): error " << error.c_str());
    this->SetLastErrorText(error.c_str());
    SQLFreeHandle(SQL_HANDLE_STMT, statement);
    return this->Record;
    }
  while (status == SQL_SUCCESS)
    {
    vtkStdString fieldName = odbcGetString(statement, 3, -1);
    this->Record->InsertNextValue(fieldName);
    status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
    }

  status = SQLFreeHandle(SQL_HANDLE_STMT, statement);
  if (status != SQL_SUCCESS)
    {
    vtkErrorMacro("vtkODBCDatabase: Unable to free statement handle: error "
                  << status);
    }

  return this->Record;
}

// ----------------------------------------------------------------------------
void vtkODBCDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataSourceName: ";
  if(this->DataSourceName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->DataSourceName << endl;
    }

  os << indent << "DatabaseName: ";
  if(this->DatabaseName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->DatabaseName << endl;
    }

  os << indent << "UserName: ";
  if(this->UserName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->UserName << endl;
    }
  os << indent << "HostName: ";
  if(this->HostName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->HostName << endl;
    }
  os << indent << "Password: ";
  if(this->Password==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << "not displayed for security reason." << endl;
    }
  os << indent << "ServerPort: " << this->ServerPort << endl;

  os << indent << "DatabaseType: "
     << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::HasError()
{
  return this->LastErrorText != NULL;
}

// ----------------------------------------------------------------------------
vtkStdString vtkODBCDatabase::GetURL()
{
  return vtkStdString("GetURL on ODBC databases is not yet implemented");
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::ParseURL(const char *URL)
{
  std::string urlstr( URL ? URL : "" );
  std::string protocol;
  std::string username;
  std::string unused;
  std::string dsname;
  std::string dataport;
  std::string database;

  // Okay now for all the other database types get more detailed info
  if ( ! vtksys::SystemTools::ParseURL(
      urlstr, protocol, username, unused, dsname, dataport, database) )
    {
    vtkErrorMacro( "Invalid URL: \"" << urlstr.c_str() << "\"" );
    return false;
    }

  if ( protocol == "odbc" )
    {
    this->SetUserName(username.c_str());
    this->SetServerPort(atoi(dataport.c_str()));
    this->SetDatabaseName(database.c_str());
    this->SetDataSourceName(dsname.c_str());
    return true;
    }

  return false;
}

// ----------------------------------------------------------------------------
vtkStdString vtkODBCDatabase::GetColumnSpecification(
  vtkSQLDatabaseSchema* schema,
  int tblHandle,
  int colHandle)
{
  vtksys_ios::ostringstream queryStr;
  queryStr << schema->GetColumnNameFromHandle( tblHandle, colHandle ) << " ";

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle );
  vtkStdString colTypeStr;

  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:
      colTypeStr = "INTEGER NOT NULL";
      break;
    case vtkSQLDatabaseSchema::SMALLINT:
      colTypeStr = "SMALLINT";
      break;
    case vtkSQLDatabaseSchema::INTEGER:
      colTypeStr = "INT";
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
      colTypeStr = "FLOAT";
      break;
    case vtkSQLDatabaseSchema::DOUBLE:
      colTypeStr = "DOUBLE PRECISION";
      break;
    case vtkSQLDatabaseSchema::BLOB:
      colTypeStr = "BLOB";
      break;
    case vtkSQLDatabaseSchema::TIME:
      colTypeStr = "TIME";
      break;
    case vtkSQLDatabaseSchema::DATE:
      colTypeStr = "DATE";
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP:
      colTypeStr = "TIMESTAMP";
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
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::INTEGER:
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::BIGINT:
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::VARCHAR:
      colSizeType = -1;
      break;
    case vtkSQLDatabaseSchema::TEXT:
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::REAL:
      colSizeType =  0; // Eventually will make DB schemata handle (M,D) sizes
      break;
    case vtkSQLDatabaseSchema::DOUBLE:
      colSizeType =  0; // Eventually will make DB schemata handle (M,D) sizes
      break;
    case vtkSQLDatabaseSchema::BLOB:
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::TIME:
      colSizeType =  0;
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

// ----------------------------------------------------------------------------
vtkStdString vtkODBCDatabase::GetIndexSpecification(
  vtkSQLDatabaseSchema* schema,
  int tblHandle,
  int idxHandle,
  bool& skipped)
{
  skipped = false;
  vtkStdString queryStr = ", ";
  bool mustUseName = true;

  int idxType = schema->GetIndexTypeFromHandle( tblHandle, idxHandle );
  switch ( idxType )
    {
    case vtkSQLDatabaseSchema::PRIMARY_KEY:
      queryStr += "PRIMARY KEY ";
      mustUseName = false;
      break;
    case vtkSQLDatabaseSchema::UNIQUE:
      queryStr += "UNIQUE ";
      break;
    case vtkSQLDatabaseSchema::INDEX:
      queryStr += "INDEX ";
      break;
    default:
      return vtkStdString();
    }

  // No index_name for PRIMARY KEYs
  if ( mustUseName )
    {
    queryStr += schema->GetIndexNameFromHandle( tblHandle, idxHandle );
    }
  queryStr += " (";

  // Loop over all column names of the index
  int numCnm = schema->GetNumberOfColumnNamesInIndex( tblHandle, idxHandle );
  if ( numCnm < 0 )
    {
    vtkGenericWarningMacro( "Unable to get index specification: index has incorrect number of columns " << numCnm );
    return vtkStdString();
    }

  bool firstCnm = true;
  for ( int cnmHandle = 0; cnmHandle < numCnm; ++ cnmHandle )
    {
    if ( firstCnm )
      {
      firstCnm = false;
      }
    else
      {
      queryStr += ",";
      }
    queryStr += schema->GetIndexColumnNameFromHandle( tblHandle, idxHandle, cnmHandle );
    }
  queryStr += ")";

  return queryStr;
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::CreateDatabase(const char *dbName,
                                     bool dropExisting = false )
{
  if ( dropExisting )
    {
    this->DropDatabase( dbName );
    }
  vtkStdString queryStr;
  queryStr = "CREATE DATABASE ";
  queryStr += dbName;
  vtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery( queryStr.c_str() );
  bool status = query->Execute();
  query->Delete();
  // Close and re-open in case we deleted and recreated the current database
  this->Close();
  this->Open( this->Password );
  return status;
}

// ----------------------------------------------------------------------------
bool vtkODBCDatabase::DropDatabase(const char *dbName)
{
  vtkStdString queryStr;
  queryStr = "DROP DATABASE ";
  queryStr += dbName;
  vtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery( queryStr.c_str() );
  bool status = query->Execute();
  query->Delete();
  return status;
}
