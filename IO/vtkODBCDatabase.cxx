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
  ----------------------------------------------------------------------------*/

/* 
 * Microsoft's own version of sqltypes.h relies on some typedefs and
 * macros in windows.h.  This next fragment tells VTK to include the
 * whole thing without any of its usual #defines to keep the size
 * manageable.  No WIN32_LEAN_AND_MEAN for us!
 */
#if defined(_WIN32) && !defined(__CYGWIN__) 
# include <vtkWindows.h> 
#endif


#include "vtkODBCDatabase.h"
#include "vtkODBCQuery.h"
#include "vtkODBCInternals.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#include <assert.h>
#include <string.h>

#include <sql.h>
#include <sqlext.h>


// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkODBCDatabase, "1.1");
vtkStandardNewMacro(vtkODBCDatabase);

// ----------------------------------------------------------------------


static vtkStdString
GetErrorMessage(SQLSMALLINT handleType, SQLHANDLE handle, int *code=0)
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

// ----------------------------------------------------------------------
 
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
                        (SQLPOINTER) buffer,
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

// ----------------------------------------------------------------------

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

  this->Port = -1; // use whatever the driver defaults to
}

// ----------------------------------------------------------------------

vtkODBCDatabase::~vtkODBCDatabase()
{
  if ( this->IsOpen() )
    {
    this->Close();
    }

  this->SetLastErrorText(NULL);
  this->SetUserName(NULL);
  this->SetHostName(NULL);
  this->SetDataSourceName(NULL);
  this->SetDatabaseName(NULL);
  delete this->Internals;

  this->Tables->UnRegister(this);
  this->Record->UnRegister(this);
}

// ----------------------------------------------------------------------

bool 
vtkODBCDatabase::IsSupported(int feature)
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

// ----------------------------------------------------------------------

bool 
vtkODBCDatabase::Open(const char* password)
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
                           (SQLPOINTER) SQL_OV_ODBC3, 
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

  if (this->UserName != NULL)
    {
    connectionString += ";UID=";
    connectionString += this->UserName;
    }
  if (password != NULL)
    {
    connectionString += ";PWD=";
    connectionString += password;
    }
  if (this->DatabaseName != NULL)
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


  // XXX Remove this once it's working; it prints passwords!
  vtkDebugMacro(<<"vtkODBCDatabase::Open: Connection string is "
                << connectionString.c_str());

  SQLSMALLINT cb;
  SQLTCHAR connectionOut[1024];
  status = SQLDriverConnect(this->Internals->Connection,
                            NULL,
                            (SQLCHAR *)connectionString.c_str(),
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

// ----------------------------------------------------------------------

void 
vtkODBCDatabase::Close()
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

// ----------------------------------------------------------------------

bool 
vtkODBCDatabase::IsOpen()
{
  return (this->Internals->Connection != SQL_NULL_HDBC);
}

// ----------------------------------------------------------------------

vtkSQLQuery* 
vtkODBCDatabase::GetQueryInstance()
{
  vtkODBCQuery *query = vtkODBCQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------

const char* 
vtkODBCDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------

vtkStringArray* 
vtkODBCDatabase::GetTables()
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
                       (SQLCHAR *)tableType.c_str(), 
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

// ----------------------------------------------------------------------

vtkStringArray* 
vtkODBCDatabase::GetRecord(const char *table)
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
                      (SQLCHAR *)table,
                      strlen(table), 
                      NULL, // column
                      0);
  
  if (status != SQL_SUCCESS)
    {
    vtkErrorMacro(<<"vtkODBCDatabase::GetRecord: Unable to retrieve column list: error "
                  << status);
    }
  
  status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
  if (status != SQL_SUCCESS)
    {
    vtkErrorMacro(<<"vtkODBCDatabase::GetRecord: Unable to retrieve column list: error "
                  << status);
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
                          
// ----------------------------------------------------------------------
                          
void 
vtkODBCDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

bool
vtkODBCDatabase::HasError()
{
  return (this->LastErrorText != NULL);
}

// ----------------------------------------------------------------------

vtkStdString
vtkODBCDatabase::GetURL()
{
  return vtkStdString("GetURL on ODBC databases is not yet implemented");
}

// ----------------------------------------------------------------------

bool
vtkODBCDatabase::ParseURL(const char *URL)
{
  vtkstd::string protocol;
  vtkstd::string username; 
  vtkstd::string unused;
  vtkstd::string dsname; 
  vtkstd::string dataport; 
  vtkstd::string database;

  // Okay now for all the other database types get more detailed info
  if ( ! vtksys::SystemTools::ParseURL( URL, protocol, username,
                                        unused, dsname, dataport, database) )
    {
    vtkErrorMacro( "Invalid URL: " << URL );
    return false;
    }
  
  if ( protocol == "odbc" )
    {
    this->SetUserName(username.c_str());
    this->SetPort(atoi(dataport.c_str()));
    this->SetDatabaseName(database.c_str());
    this->SetDataSourceName(dsname.c_str());
    return true;
    }

  return false;
}
