/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkODBCQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* 
 * Microsoft's own version of sqltypes.h relies on some typedefs and
 * macros in windows.h.  This next fragment tells VTK to include the
 * whole thing without any of its usual #defines to keep the size
 * manageable.  No WIN32_LEAN_AND_MEAN for us!
 */
#if defined(_WIN32) && !defined(__CYGWIN__) 
# define VTK_WINDOWS_FULL 
# include <vtkWindows.h> 
#endif


#include "vtkODBCQuery.h"
#include "vtkODBCDatabase.h"
#include "vtkODBCInternals.h"

#include <vtkBitArray.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>

#include <assert.h>

#include <vtksys/ios/sstream>
#include <vtksys/stl/vector>

#include <sql.h>
#include <sqlext.h>

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkODBCQuery);

static vtkStdString GetErrorMessage(SQLSMALLINT handleType, SQLHANDLE handle, int *code=0);


/*
 * Bound Parameters and ODBC
 *
 * ODBC handles bound parameters by requiring that the user pass in
 * buffers containing data for each parameter to be bound.  These are
 * bound to the statement using SQLBindParam.  The statement must have
 * been prepared using SQLPrepare.  Those buffers need to be freed
 * when they're no longer needed.
 * 
 * I'm going to handle this by using my own class
 * (vtkODBCBoundParameter) to hold all the information the user passes
 * in.  This is the same sort of trick I use for the MySQL bound
 * parameter support.  At execution time I'll take the parameters and
 * attach them all to the statement.  The vtkODBCBoundParameter
 * instances will each own the buffesr for their data.
 *
 * This is slightly inefficient in that it will generate
 * a few tiny little new[] requests.  If this ever becomes a problem,
 * we can allocate a fixed-size buffer (8 or 16 bytes) inside
 * vtkODBCBoundParameter and use that for the data storage by
 * default.  That will still require special-case handling for blobs
 * and strings.
 *
 * The vtkODBCQueryInternals class will handle the bookkeeping for
 * which parameters are and aren't bound at any given time.
 */

// ----------------------------------------------------------------------

class vtkODBCBoundParameter
{
public:
  vtkODBCBoundParameter() :
    Data(NULL), DataLength(0), BufferSize(0), DataTypeC(0), DataTypeSQL(0)
    {
    }

  ~vtkODBCBoundParameter()
    {
      delete [] this->Data;
    }

  void SetData(const char *data, unsigned long size)
    {
      delete [] this->Data;
      this->BufferSize = size;
      this->DataLength = size;
      this->Data = new char[size];
      memcpy(this->Data, data, size);
    }

public:
  char             *Data;        // Buffer holding actual data
  unsigned long     DataLength; 
  SQLLEN            BufferSize; // will be at least as large as DataLength
  SQLSMALLINT       DataTypeC;
  SQLSMALLINT       DataTypeSQL;
};

// ----------------------------------------------------------------------

class vtkODBCQueryInternals
{
public:
  vtkODBCQueryInternals() 
    { 
      this->Statement = NULL;
      this->CurrentRow = vtkVariantArray::New();
      this->ColumnNames = vtkStringArray::New();
      this->ColumnIsSigned = vtkBitArray::New();
      this->ColumnTypes = NULL;
      this->NullPermitted = vtkBitArray::New();
    }

  ~vtkODBCQueryInternals()
    {
      this->FreeUserParameterList();
      if (this->Statement != NULL)
        {
        SQLFreeHandle(SQL_HANDLE_STMT, this->Statement);
        }
      this->CurrentRow->Delete();
      this->ColumnNames->Delete();
      this->ColumnIsSigned->Delete();
      this->NullPermitted->Delete();
      if (this->ColumnTypes)
        {
        delete [] this->ColumnTypes;
        }
    }

  void FreeStatement();
  void FreeUserParameterList();
  void ClearBoundParameters();
  bool PrepareQuery(const char *queryString, SQLHANDLE connection, vtkStdString &error_message);
  bool SetBoundParameter(int index, vtkODBCBoundParameter *param);
  bool BindParametersToStatement();

public:
  SQLHANDLE Statement;
  vtkStdString Name;

  vtkVariantArray *CurrentRow;
  vtkStringArray *ColumnNames;
  vtkBitArray *ColumnIsSigned;
  vtkBitArray *NullPermitted;
  SQLSMALLINT *ColumnTypes;

  typedef vtksys_stl::vector<vtkODBCBoundParameter *> ParameterList;
  ParameterList UserParameterList;

};

// ----------------------------------------------------------------------

void
vtkODBCQueryInternals::FreeStatement()
{
  if (this->Statement)
    {
    SQLRETURN status;
    status = SQLCloseCursor(this->Statement);
    if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
      {
      vtksys_ios::ostringstream errorBuf;
      errorBuf << "vtkODBCQuery: Unable to close SQL cursor.  Error: "
               << GetErrorMessage(SQL_HANDLE_STMT, this->Statement);
      cerr << errorBuf.str() << "\n";
      }
    
    status = SQLFreeHandle(SQL_HANDLE_STMT, this->Statement);
    if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
      {
      vtksys_ios::ostringstream errorBuf;
      errorBuf << "Unable to free statement handle.  Memory leak will occur. Error: "
               << GetErrorMessage(SQL_HANDLE_STMT, this->Statement);
      cerr << errorBuf.str() << "\n";
      }
    this->Statement = 0;
    }
}

// ----------------------------------------------------------------------

#define VTK_ODBC_C_TYPENAME_MACRO(type,return_type) \
  SQLSMALLINT vtkODBCTypeNameC(type) \
  { return return_type; }

VTK_ODBC_C_TYPENAME_MACRO(signed char, SQL_C_STINYINT);
VTK_ODBC_C_TYPENAME_MACRO(unsigned char, SQL_C_UTINYINT);
VTK_ODBC_C_TYPENAME_MACRO(signed short, SQL_C_SSHORT);
VTK_ODBC_C_TYPENAME_MACRO(unsigned short, SQL_C_USHORT);
VTK_ODBC_C_TYPENAME_MACRO(signed int, SQL_C_SLONG);
VTK_ODBC_C_TYPENAME_MACRO(unsigned int, SQL_C_ULONG);
VTK_ODBC_C_TYPENAME_MACRO(signed long, SQL_C_SLONG);
VTK_ODBC_C_TYPENAME_MACRO(unsigned long, SQL_C_ULONG);
VTK_ODBC_C_TYPENAME_MACRO(float, SQL_C_FLOAT);
VTK_ODBC_C_TYPENAME_MACRO(double, SQL_C_DOUBLE);
VTK_ODBC_C_TYPENAME_MACRO(vtkTypeInt64, SQL_C_SBIGINT);
VTK_ODBC_C_TYPENAME_MACRO(vtkTypeUInt64, SQL_C_UBIGINT);
VTK_ODBC_C_TYPENAME_MACRO(const char *, SQL_C_CHAR);
VTK_ODBC_C_TYPENAME_MACRO(char *, SQL_C_CHAR);
VTK_ODBC_C_TYPENAME_MACRO(unsigned char *, SQL_C_CHAR);
VTK_ODBC_C_TYPENAME_MACRO(void *, SQL_C_BINARY);


#define VTK_ODBC_SQL_TYPENAME_MACRO(type,return_type) \
  SQLSMALLINT vtkODBCTypeNameSQL(type) \
  { return return_type; }

VTK_ODBC_SQL_TYPENAME_MACRO(signed char, SQL_TINYINT);
VTK_ODBC_SQL_TYPENAME_MACRO(unsigned char, SQL_TINYINT);
VTK_ODBC_SQL_TYPENAME_MACRO(signed short, SQL_SMALLINT);
VTK_ODBC_SQL_TYPENAME_MACRO(unsigned short, SQL_SMALLINT);
VTK_ODBC_SQL_TYPENAME_MACRO(signed int, SQL_INTEGER);
VTK_ODBC_SQL_TYPENAME_MACRO(unsigned int, SQL_INTEGER);
VTK_ODBC_SQL_TYPENAME_MACRO(signed long, SQL_INTEGER);
VTK_ODBC_SQL_TYPENAME_MACRO(unsigned long, SQL_INTEGER);
VTK_ODBC_SQL_TYPENAME_MACRO(float, SQL_REAL);
VTK_ODBC_SQL_TYPENAME_MACRO(double, SQL_DOUBLE);
VTK_ODBC_SQL_TYPENAME_MACRO(vtkTypeInt64, SQL_BIGINT);
VTK_ODBC_SQL_TYPENAME_MACRO(vtkTypeUInt64, SQL_BIGINT);
VTK_ODBC_SQL_TYPENAME_MACRO(const char *, SQL_VARCHAR);
VTK_ODBC_SQL_TYPENAME_MACRO(char *, SQL_VARCHAR);
VTK_ODBC_SQL_TYPENAME_MACRO(unsigned char *, SQL_VARCHAR);
VTK_ODBC_SQL_TYPENAME_MACRO(void *, SQL_VARBINARY);

// ----------------------------------------------------------------------

// Description:
// This function will build and populate a vtkODBCBoundParameter
// struct.  The default implementation works for POD data types (char,
// int, long, etc).  I'll need to special-case strings and blobs.

template<typename T>
vtkODBCBoundParameter *vtkBuildODBCBoundParameter(T data_value)
{
  vtkODBCBoundParameter *param = new vtkODBCBoundParameter;

  param->DataTypeC =   vtkODBCTypeNameC(data_value);
  param->DataTypeSQL = vtkODBCTypeNameSQL(data_value);
  param->BufferSize = sizeof(T);
  param->DataLength = sizeof(T);
  param->SetData(reinterpret_cast<const char *>(&data_value),
                 sizeof(T));
  
  return param;
}

// Description:
// Specialization of vtkBuildBoundParameter for NULL-terminated
// strings (i.e. CHAR and VARCHAR fields)

template<>
vtkODBCBoundParameter *vtkBuildODBCBoundParameter<const char *>(const char *data_value)
{
  vtkODBCBoundParameter *param = new vtkODBCBoundParameter;

  param->DataTypeC = SQL_C_CHAR;
  param->DataTypeSQL = SQL_VARCHAR;
  param->BufferSize = strlen(data_value);
  param->DataLength = strlen(data_value);
  param->SetData(data_value, strlen(data_value));

  return param;
}

// Description:
// Alternate signature for vtkBuildBoundParameter to handle blobs

vtkODBCBoundParameter *vtkBuildODBCBoundParameter(const char *data,
                                                  unsigned long length,
                                                  bool is_blob)
{
  vtkODBCBoundParameter *param = new vtkODBCBoundParameter;

  param->DataTypeC = SQL_C_CHAR;
  if (is_blob)
    {
    param->DataTypeSQL = SQL_VARBINARY;
    }
  else
    {
    param->DataTypeSQL = SQL_VARCHAR;
    }
  param->BufferSize = length; 
  param->DataLength = length; 
  param->SetData(data, length);

  return param;
}

// ----------------------------------------------------------------------

bool vtkODBCQueryInternals::PrepareQuery(const char *queryString, 
                                         SQLHANDLE dbConnection,
                                         vtkStdString &error_message)
{
  this->FreeStatement();
  this->FreeUserParameterList();

  // ODBC requires that drivers either support query preparation or
  // emulate it to the greatest extent possible.  It says nothing
  // about what queries may or may not be prepared.  I'm going to
  // close my eyes and pretend that all SQL is valid for preparation
  // even if bound parameters don't make sense.  If I'm wrong the
  // error messages will certainly tell me so.

  SQLRETURN status;
  status = SQLAllocHandle(SQL_HANDLE_STMT, 
                          dbConnection,
                          &(this->Statement));
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream errorBuf;
    errorBuf << "Unable to allocate new statement handle.  Error: "
             << GetErrorMessage(SQL_HANDLE_DBC, dbConnection);
    error_message = errorBuf.str();
    return false;
    }

  // Queries in VTK currently only support scrolling forward through
  // the results, not forward/backward/randomly.
  status = SQLSetStmtAttr(this->Statement,
                          SQL_ATTR_CURSOR_TYPE,
                          static_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY),
                          SQL_IS_UINTEGER);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    error_message = vtkStdString(GetErrorMessage(SQL_HANDLE_STMT,
                                                 this->Statement));
    return false;
    }

  // ugh, I hate having to use const_cast
  status = SQLPrepare(this->Statement,
                      reinterpret_cast<SQLCHAR *>(const_cast<char *>(queryString)),
                      strlen(queryString));

  if (status != SQL_SUCCESS)
    {
    vtksys_ios::ostringstream errorBuf;
    errorBuf << "Unable to prepare query for execution: "
             << GetErrorMessage(SQL_HANDLE_STMT, this->Statement);
    error_message = errorBuf.str();
    return false;
    }
  else
    {
    error_message = vtkStdString();
    SQLSMALLINT paramCount;
    status = SQLNumParams(this->Statement, &paramCount);
    if (status != SQL_SUCCESS)
      {
      error_message = vtkStdString(GetErrorMessage(SQL_HANDLE_STMT,
                                                   this->Statement));
      return false;
      }
    else
      {
      this->UserParameterList.resize(paramCount, NULL);
      return true;
      }
    }
}

// ----------------------------------------------------------------------

void vtkODBCQueryInternals::FreeUserParameterList()
{
  for (unsigned int i = 0; i < this->UserParameterList.size(); ++i)
    {
    if (this->UserParameterList[i] != NULL)
      {
      delete this->UserParameterList[i];
      this->UserParameterList[i] = NULL;
      }
    }
  this->UserParameterList.clear();
}

// ----------------------------------------------------------------------

bool vtkODBCQueryInternals::SetBoundParameter(int index, vtkODBCBoundParameter *param)
{
  if (index >= static_cast<int>(this->UserParameterList.size()))
    {
    vtkGenericWarningMacro(<<"ERROR: Illegal parameter index "
                           <<index << ".  Did you forget to set the query?");
    return false;
    }
  else
    {
    if (this->UserParameterList[index] != NULL)
      {
      delete this->UserParameterList[index];
      }
    this->UserParameterList[index] = param;
    return true;
    }
}

// ----------------------------------------------------------------------

void vtkODBCQueryInternals::ClearBoundParameters()
{
  if (this->Statement)
    {
    SQLFreeStmt(this->Statement, SQL_RESET_PARAMS);
    }
}

// ----------------------------------------------------------------------

bool vtkODBCQueryInternals::BindParametersToStatement()
{
  if (this->Statement == NULL)
    {
    vtkGenericWarningMacro(<<"BindParametersToStatement: No prepared statement available");
    return false;
    }
  
  this->ClearBoundParameters();
  unsigned long numParams = this->UserParameterList.size();
  for (SQLUSMALLINT i = 0; i < numParams; ++i)
    {
    if (this->UserParameterList[i])
      {
      SQLRETURN status = SQLBindParameter(this->Statement,
                                          i+1, // parameter indexing starts at 1
                                          SQL_PARAM_INPUT,
                                          this->UserParameterList[i]->DataTypeC,
                                          this->UserParameterList[i]->DataTypeSQL,
                                          0, // column size is irrelevant
                                          0, // decimal digits are irrelevant
                                          reinterpret_cast<SQLPOINTER>(this->UserParameterList[i]->Data),
                                          this->UserParameterList[i]->DataLength,
                                          &(this->UserParameterList[i]->BufferSize));

      if (status != SQL_SUCCESS)
        {
        vtkGenericWarningMacro(<<"Unable to bind parameter "
                               << i << " to SQL statement!  Return code: "
                               << status);
        return false;
        }
      }
    }
  return true;
}

// ----------------------------------------------------------------------

static vtkStdString
GetErrorMessage(SQLSMALLINT handleType, SQLHANDLE handle, int *code)
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
        messagebuf << ", "
;
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

vtkODBCQuery::vtkODBCQuery() 
{
  this->Internals = new vtkODBCQueryInternals;
  this->InitialFetch = true;
  this->LastErrorText = NULL;
  this->QueryText = NULL;
}

// ----------------------------------------------------------------------

vtkODBCQuery::~vtkODBCQuery()
{
  this->SetLastErrorText(NULL);
  this->SetQueryText(NULL);
  delete this->Internals;
}

// ----------------------------------------------------------------------

void
vtkODBCQuery::PrintSelf(ostream  &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::SetQuery(const char *newQuery)
{
  this->Active = false;
  this->SetQueryText(newQuery);

  vtkODBCDatabase *db = vtkODBCDatabase::SafeDownCast(this->Database);
  if (db == NULL)
    {
    vtkErrorMacro(<<"SHOULDN'T HAPPEN: SetQuery called with null database.  This can only happen when you instantiate vtkODBCQuery directly.  You should always call vtkODBCDatabase::GetQueryInstance to make a query object.");
    return false;
    }

  vtkStdString error;
  bool prepareStatus = this->Internals->PrepareQuery(newQuery, db->Internals->Connection, error);
  if (prepareStatus)
    {
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtkErrorMacro(<< error.c_str());
    this->SetLastErrorText(error.c_str());
    return false;
    }
}

// ----------------------------------------------------------------------

const char *
vtkODBCQuery::GetQuery()
{
  return this->GetQueryText();
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::Execute()
{
  // It's possible to call this function while a cursor is still open.
  // This is not an error, but we do need to close out the previous
  // cursor before opening up a new one.
  this->Active = false;
  SQLFreeStmt(this->Internals->Statement, SQL_CLOSE);

  this->Internals->BindParametersToStatement();
  
  SQLRETURN status = SQLExecute(this->Internals->Statement);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream errorBuf;

    errorBuf << "Unable to execute statement: "
             << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
    
    this->SetLastErrorText(errorBuf.str().c_str());
    this->Active = false;
    return false;
    }
  else
    {
    vtkDebugMacro(<<"SQLExecute succeeded.");

    this->Active = true;

    this->Internals->ColumnNames->Reset();
    this->Internals->CurrentRow->Reset();
    this->Internals->ColumnIsSigned->Reset();
    this->Internals->NullPermitted->Reset();
    if (this->Internals->ColumnTypes)
      {
      delete [] this->Internals->ColumnTypes; 
      this->Internals->ColumnTypes = NULL;
      }

    // Populate the result information now, all at once, rather than
    // making a whole bunch of calls later and duplicating
    // (potentially expensive) operations.
    int numColumns = this->GetNumberOfFields();
    if (numColumns)
      {
      this->Internals->ColumnTypes = new SQLSMALLINT[numColumns];
      this->Internals->NullPermitted->SetNumberOfTuples(numColumns);
      this->Internals->CurrentRow->SetNumberOfTuples(numColumns);
      this->Internals->ColumnNames->SetNumberOfTuples(numColumns);
      this->Internals->ColumnIsSigned->SetNumberOfTuples(numColumns);

      for (int i = 0; i < numColumns; ++i)
        {
        SQLCHAR name[1024];
        SQLSMALLINT nameLength;
        SQLSMALLINT dataType;
        SQLULEN columnSize;
        SQLSMALLINT decimalDigits;
        SQLSMALLINT nullable;
        SQLLEN unsignedFlag = SQL_FALSE;

        status = SQLDescribeCol(this->Internals->Statement,
                                i + 1, // 1-indexed, not 0
                                name,
                                1024,
                                &nameLength,
                                &dataType,
                                &columnSize,
                                &decimalDigits,
                                &nullable);
    
        if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
          {
          vtksys_ios::ostringstream errbuf;
          errbuf << "During vtkODBCQuery::Execute while looking up column "
                 << i << ": "
                 << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
          this->SetLastErrorText(errbuf.str().c_str());
          vtkErrorMacro(<< errbuf.str().c_str());
          }

        status = SQLColAttribute(this->Internals->Statement,
                                 i+1,
                                 SQL_DESC_UNSIGNED,
                                 0,
                                 0,
                                 0,
                                 &unsignedFlag);

        if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
          {
          vtksys_ios::ostringstream errbuf;
          errbuf << "vtkODBCQuery::Execute: Unable to get unsigned flag for column "
                 << i << ": " 
                 << GetErrorMessage(SQL_HANDLE_STMT,
                                    this->Internals->Statement);
          this->SetLastErrorText(errbuf.str().c_str());
          vtkErrorMacro(<< errbuf.str().c_str());
          }

        this->Internals->ColumnNames->SetValue(i,reinterpret_cast<const char *>(name));
        this->Internals->ColumnIsSigned->SetValue(i, (unsignedFlag == SQL_FALSE));
        this->Internals->ColumnTypes[i] = dataType;
        this->Internals->NullPermitted->SetValue(i, nullable);
        } // done populating column information
      }
    this->SetLastErrorText(NULL);
    return true;
    }
}

// ----------------------------------------------------------------------

int
vtkODBCQuery::GetNumberOfFields()
{
  if (!this->Active)
    {
    return 0;
    }

  SQLSMALLINT count;
  SQLRETURN status;

  status = SQLNumResultCols(this->Internals->Statement, &count);
  if (status != SQL_SUCCESS &&
      status != SQL_SUCCESS_WITH_INFO)
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "During vtkODBCQuery::GetNumberOfFields: "
           << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    return 0;
    }

  this->SetLastErrorText(NULL);
  return count;
}


// ----------------------------------------------------------------------

const char *
vtkODBCQuery::GetFieldName(int column)
{
  if (! this->Active)
    {
    vtkErrorMacro(<<"GetFieldName(): Query is not active!");
    return NULL;
    }
  else if (column < 0 || column >= this->GetNumberOfFields())
    {
    vtkErrorMacro(<<"GetFieldName(): Illegal field index " 
                  << column);
    return NULL;
    }
  else
    {
    return this->Internals->ColumnNames->GetValue(column).c_str();
    }
}

// ----------------------------------------------------------------------

int
vtkODBCQuery::GetFieldType(int column)
{
  if (! this->Active)
    {
    vtkErrorMacro(<<"GetFieldType(): Query is not active!");
    return VTK_VOID;
    }
  else if (column < 0 || column >= this->GetNumberOfFields())
    {
    vtkErrorMacro(<<"GetFieldType(): Illegal field index " 
                  << column);
    return VTK_VOID;
    }
  else
    {

    switch (this->Internals->ColumnTypes[column])
      {
      case SQL_CHAR:
      case SQL_VARCHAR:
      case SQL_LONGVARCHAR:
      case SQL_WCHAR:
      case SQL_WVARCHAR:
      case SQL_WLONGVARCHAR:
        return VTK_STRING;

      case SQL_INTEGER:
      case SQL_NUMERIC:
        if (this->Internals->ColumnIsSigned->GetValue(column))
          {
          return VTK_INT;
          }
        else
          {
          return VTK_UNSIGNED_INT;
          }

      case SQL_TINYINT:
        if (this->Internals->ColumnIsSigned->GetValue(column))
          {
          return VTK_SIGNED_CHAR;
          }
        else
          {
          return VTK_UNSIGNED_CHAR;
          }

      case SQL_SMALLINT:
        if (this->Internals->ColumnIsSigned->GetValue(column))
          {
          return VTK_SHORT;
          }
        else
          {
          return VTK_UNSIGNED_SHORT;
          }

      case SQL_BIT:
        return VTK_BIT;

      case SQL_REAL:
      case SQL_FLOAT:
        return VTK_FLOAT;

      case SQL_DOUBLE:
        return VTK_DOUBLE;

      case SQL_BINARY:
      case SQL_VARBINARY:
      case SQL_LONGVARBINARY:
        return VTK_STRING;

      case SQL_BIGINT:
      case SQL_DECIMAL:
        return VTK_TYPE_INT64;

      case SQL_TYPE_TIMESTAMP:
//      case SQL_TYPE_UTCDATETIME:
//      case SQL_TYPE_UTCTIME:
      case SQL_TYPE_DATE:
      case SQL_TYPE_TIME:
        return VTK_TYPE_UINT64;
        
      case SQL_INTERVAL_MONTH:
      case SQL_INTERVAL_YEAR:
      case SQL_INTERVAL_DAY:
      case SQL_INTERVAL_HOUR:
      case SQL_INTERVAL_MINUTE:
      case SQL_INTERVAL_SECOND:
        return VTK_TYPE_UINT64;

        // unhandled: SQL_INTERVAL_YEAR_TO_MONTH,
        // SQL_INTERVAL_DAY_TO_HOUR, SQL_INTERVAL_DAY_TO_MINUTE,
        // SQL_INTERVAL_DAY_TO_SECOND, SQL_INTERVAL_HOUR_TO_MINUTE,
        // SQL_INTERVAL_HOUR_TO_SECOND, SQL_INTERVAL_MINUTE_TO_SECOND,
        // SQL_GUID

      default:
      {
      vtkWarningMacro(<<"Unknown type " << this->Internals->ColumnTypes[column]
                      <<" returned from SQLDescribeCol");
      return VTK_VOID;
      }
      }
    }
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::NextRow()
{
  if (! this->IsActive())
    {
    vtkErrorMacro(<<"NextRow(): Query is not active!");
    return false;
    }

  this->ClearCurrentRow();

  SQLRETURN status = SQLFetch(this->Internals->Statement);
  if (status == SQL_SUCCESS)
    {
    this->SetLastErrorText(NULL);
    return this->CacheCurrentRow();
    }
  else if (status == SQL_NO_DATA)
    {
    this->SetLastErrorText(NULL);
    return false;
    }
  else
    {
    return false;
    }
}

// ----------------------------------------------------------------------

vtkVariant
vtkODBCQuery::DataValue(vtkIdType column)
{
  if (this->IsActive() == false)
    {
    vtkWarningMacro(<<"DataValue() called on inactive query");
    return vtkVariant();
    }
  else if (column < 0 || column >= this->GetNumberOfFields())
    {
    vtkWarningMacro(<<"DataValue() called with out-of-range column index "
                    << column);
    return vtkVariant();
    }
  else
    {
    return this->Internals->CurrentRow->GetValue(column);
    }
}

// ----------------------------------------------------------------------

void
vtkODBCQuery::ClearCurrentRow()
{
  for (vtkIdType i = 0; 
       i < this->Internals->CurrentRow->GetNumberOfTuples(); 
       ++i)
    {
    this->Internals->CurrentRow->SetValue(i, vtkVariant());
    }
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheCurrentRow()
{
  int column;
  bool status = true;

  for (column = 0; column < this->GetNumberOfFields(); ++column)
    {
    switch (this->Internals->ColumnTypes[column])
      {
      case SQL_CHAR:
      case SQL_VARCHAR:
      case SQL_LONGVARCHAR:
        status = status & this->CacheStringColumn(column);
        break;

      case SQL_WCHAR:
      case SQL_WVARCHAR:
      case SQL_WLONGVARCHAR:
        status = status & this->CacheWideStringColumn(column);
        break;

      case SQL_DECIMAL:
        status = status & this->CacheDecimalColumn(column);
        break;

      case SQL_NUMERIC:
        status = status & this->CacheNumericColumn(column);
        break;

      case SQL_SMALLINT:
      case SQL_INTEGER:
        status = status & this->CacheIntColumn(column);
        break;
        
      case SQL_REAL:
      case SQL_FLOAT:
        status = status & this->CacheFloatColumn(column);
        break;

      case SQL_DOUBLE:
        status = status & this->CacheDoubleColumn(column);
        break;

      case SQL_BIT:
        status = status & this->CacheBooleanColumn(column);
        break;

      case SQL_TINYINT:
        status = status & this->CacheCharColumn(column);
        break;
        
      case SQL_BIGINT:
        status = status & this->CacheLongLongColumn(column);
        break;

      case SQL_BINARY:
      case SQL_VARBINARY:
      case SQL_LONGVARBINARY:
        status = status & this->CacheBinaryColumn(column);
        break;

      case SQL_TYPE_DATE:
      case SQL_TYPE_TIME:
      case SQL_TYPE_TIMESTAMP:
//      case SQL_TYPE_UTCDATETIME:
//      case SQL_TYPE_UTCTIME:
        status = status & this->CacheTimeColumn(column);
        break;

      case SQL_INTERVAL_MONTH:
      case SQL_INTERVAL_YEAR:
      case SQL_INTERVAL_DAY:
      case SQL_INTERVAL_HOUR:
      case SQL_INTERVAL_MINUTE:
      case SQL_INTERVAL_SECOND:
        status = status & this->CacheIntervalColumn(column);
        break;

      default:
      {
      vtkWarningMacro(<<"DataValue: Unsupported SQL data type "
                      << this->Internals->ColumnTypes[column]
                      << " on column " << column);
      status = false;
      this->Internals->CurrentRow->SetValue(column, vtkVariant());
      }; break;
      }
    }

  return status;
}
  
// ----------------------------------------------------------------------

const char *
vtkODBCQuery::GetLastErrorText()
{
  return this->LastErrorText; 
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::HasError()
{
  return (this->LastErrorText != NULL);
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::BeginTransaction()
{
  if (!this->Database->IsOpen())
    {
    this->SetLastErrorText("Cannot begin transaction.  Database is closed.");
    return false;
    }

  vtkODBCDatabase *db = vtkODBCDatabase::SafeDownCast(this->Database);
  assert(db != NULL);

  SQLUINTEGER ac = SQL_AUTOCOMMIT_OFF;
  SQLRETURN status = SQLSetConnectAttr(db->Internals->Connection, 
                                       SQL_ATTR_AUTOCOMMIT,
                                       reinterpret_cast<SQLPOINTER>(ac),
                                       sizeof(ac));

  if (status != SQL_SUCCESS)
    {
    this->SetLastErrorText("Unable to disable autocommit.");
    return false;
    }
  return true; 
}


// ----------------------------------------------------------------------

bool
vtkODBCQuery::CommitTransaction()
{
  if (!this->Database->IsOpen())
    {
    this->SetLastErrorText("Cannot commit transaction.  Database is closed.");
    return false;
    }

  vtkODBCDatabase *db = vtkODBCDatabase::SafeDownCast(this->Database);
  assert(db != NULL);

  SQLRETURN status;
  status = SQLEndTran(SQL_HANDLE_DBC,
                      db->Internals->Connection, 
                      SQL_COMMIT);
  if (status != SQL_SUCCESS) 
    {
    this->SetLastErrorText("Unable to commit transaction.");
    return false;
    }

  // After the transaction has ended we need to turn autocommit back
  // on so the database goes back to treating every query like a
  // transaction unto itself.
  SQLUINTEGER ac = SQL_AUTOCOMMIT_ON;
  status = SQLSetConnectAttr(db->Internals->Connection,
                             SQL_ATTR_AUTOCOMMIT,
                             reinterpret_cast<SQLPOINTER>(ac),
                             sizeof(ac));

  if (status != SQL_SUCCESS)
    {
    this->SetLastErrorText("Unable to re-enable autocommit.");
    return false;
    }

  return true;
}


// ----------------------------------------------------------------------

bool
vtkODBCQuery::RollbackTransaction()
{
  if (!this->Database->IsOpen())
    {
    this->SetLastErrorText("Cannot roll back transaction.  Database is closed.");
    return false;
    }
  
  vtkODBCDatabase *db = vtkODBCDatabase::SafeDownCast(this->Database);
  assert(db != NULL);

  SQLRETURN status;
  status = SQLEndTran(SQL_HANDLE_DBC,
                      db->Internals->Connection, 
                      SQL_ROLLBACK);
  if (status != SQL_SUCCESS) 
    {
    this->SetLastErrorText("Unable to roll back transaction.");
    return false;
    }

  // After the transaction has ended we need to turn autocommit back
  // on so the database goes back to treating every query like a
  // transaction unto itself.
  SQLUINTEGER ac = SQL_AUTOCOMMIT_ON;
  status = SQLSetConnectAttr(db->Internals->Connection,
                             SQL_ATTR_AUTOCOMMIT,
                             reinterpret_cast<SQLPOINTER>(ac),
                             sizeof(ac));

  if (status != SQL_SUCCESS)
    {
    this->SetLastErrorText("Unable to re-enable autocommit.");
    return false;
    }

  return true;
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheWideStringColumn(int column)
{
  return CacheStringColumn(column);
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheIntColumn(int column)
{
  SQLRETURN status;
  SQLINTEGER buffer;
  SQLLEN actualLength;
  SQLSMALLINT dataType;
  if (this->Internals->ColumnIsSigned->GetValue(column))
    {
    dataType = SQL_C_SLONG;
    }
  else
    {
    dataType = SQL_C_ULONG;
    }

  status = SQLGetData(this->Internals->Statement,
                      column+1,
                      dataType,
                      static_cast<SQLPOINTER>(&buffer),
                      sizeof(buffer),
                      &actualLength);
  
  if (status == SQL_SUCCESS)
    {
    vtkVariant result;
    if (this->Internals->ColumnIsSigned->GetValue(column))
      {
      result = vtkVariant(buffer);
      }
    else
      {
      unsigned int foo(buffer);
      result = vtkVariant(foo);
      }
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheIntColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return true;
    }
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheLongLongColumn(int column)
{
  SQLRETURN status;
  SQLLEN actualLength;
  int dataType;
  vtkVariant result;

  if (this->Internals->ColumnIsSigned->GetValue(column))
    {
    dataType = SQL_C_SBIGINT;
    long long buffer;

    status = SQLGetData(this->Internals->Statement,
                        column+1,
                        dataType, 
                        static_cast<SQLPOINTER>(& buffer),
                        sizeof(buffer),
                        &actualLength);
    result = vtkVariant(buffer);
    }
  else
    {
    dataType = SQL_C_UBIGINT;
    unsigned long long buffer;
    status = SQLGetData(this->Internals->Statement,
                        column+1,
                        dataType, 
                        static_cast<SQLPOINTER>(& buffer),
                        sizeof(buffer),
                        &actualLength);
    result = vtkVariant(buffer);
    }

  
  if (status == SQL_SUCCESS)
    {
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheLongLongColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return false;
    }
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheCharColumn(int column)
{
  SQLRETURN status;
  unsigned char buffer;
  SQLLEN actualLength;

  status = SQLGetData(this->Internals->Statement,
                      column+1,
                      SQL_C_TINYINT, 
                      static_cast<SQLPOINTER>(&buffer),
                      sizeof(buffer),
                      &actualLength);
  
  if (status == SQL_SUCCESS)
    {
    vtkVariant result;
    if (this->Internals->ColumnIsSigned->GetValue(column))
      {
      result = vtkVariant(buffer);
      }
    else
      {
      unsigned char foo = buffer;
      result = vtkVariant(foo);
      }
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheCharColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return false;
    }
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheBooleanColumn(int column)
{
  SQLRETURN status;
  unsigned char buffer;
  SQLLEN actualLength;

  status = SQLGetData(this->Internals->Statement,
                      column+1,
                      SQL_C_TINYINT, 
                      static_cast<SQLPOINTER>(&buffer),
                      sizeof(buffer),
                      &actualLength);
  
  if (status == SQL_SUCCESS)
    {
    vtkVariant result(buffer != 0);
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheCharColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return false;
    }
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheFloatColumn(int column)
{
  SQLRETURN status;
  SQLFLOAT buffer;
  SQLLEN actualLength;

  status = SQLGetData(this->Internals->Statement,
                      column+1,
                      (sizeof(buffer) == sizeof(double) 
                       ? SQL_C_DOUBLE
                       : SQL_C_FLOAT),
                      static_cast<SQLPOINTER>(&buffer),
                      sizeof(buffer),
                      &actualLength);

  if (status == SQL_SUCCESS)
    {
    vtkVariant result(buffer);
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheFloatColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return false;
    }
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::CacheDoubleColumn(int column)
{
  SQLRETURN status;
  SQLDOUBLE buffer;
  SQLLEN actualLength;

  status = SQLGetData(this->Internals->Statement,
                      column+1,
                      SQL_C_DOUBLE, 
                      static_cast<SQLPOINTER>(&buffer),
                      sizeof(buffer),
                      &actualLength);
  
  if (status == SQL_SUCCESS)
    {
    vtkVariant result(buffer);
    this->Internals->CurrentRow->SetValue(column, result);
    this->SetLastErrorText(NULL);
    return true;
    }
  else if (status == SQL_NULL_DATA)
    {
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(NULL);
    return true;
    }
  else
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheDoubleColumn (column "
           << column << "): ODBC error: " 
           << GetErrorMessage(SQL_HANDLE_STMT,
                              this->Internals->Statement);
    this->SetLastErrorText(errbuf.str().c_str());
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    return false; 
    }
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheStringColumn(int column)
{
  SQLRETURN status;
  char *buffer;
  SQLLEN bufferLength;
  SQLLEN indicator;
  vtkStdString result;
  vtksys_ios::ostringstream outbuf;

  bufferLength = 65536; // this is a pretty reasonable compromise
                        // between the expense of ODBC requests and
                        // application memory usage
  buffer = new char[bufferLength];

  while (true)
    {
    status = SQLGetData(this->Internals->Statement,
                        column+1,
                        SQL_C_CHAR,
                        static_cast<SQLPOINTER>(buffer),
                        bufferLength,
                        &indicator);
/*
    cerr << "once around the read loop for column " << column << ": status "
    << status << ", indicator " << indicator << "\n";
*/

    int bytesToWrite = 0;
    if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
      {
      if (status == SQL_NO_DATA)
        {
        // done reading!
        break;
        }
      else if (indicator == SQL_NULL_DATA)
        {
//        cerr << "Wide string value for column " << column << " is null\n";
        break;
        }
      // If we get to this point then there's data to read.
      if (indicator == SQL_NO_TOTAL)
        {
        bytesToWrite = bufferLength;
        }
      else if (indicator > bufferLength)
        {
        bytesToWrite = bufferLength;
        }
      else
        {
        bytesToWrite = indicator;
        }

      //      cerr << "Writing " << bytesToWrite << " characters in string column\n";
      
      if (status == SQL_SUCCESS_WITH_INFO)
        {
        // eat the null terminator
        bytesToWrite -= 1;
        }
      outbuf.write(buffer, bytesToWrite);
      if (status == SQL_SUCCESS)
        {
        // we retrieved everything in one pass
        break;
        }
      }
    else if (status == SQL_ERROR)
      {
      // there was some sort of error
      vtksys_ios::ostringstream errbuf;
      errbuf << "Error while reading wide string column " << column << ": "
             << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
      this->SetLastErrorText(errbuf.str().c_str());
      cerr << errbuf.str() << "\n";
      delete [] buffer;
      this->Internals->CurrentRow->SetValue(column, vtkVariant());
      return false;
      }
    else if (status == SQL_INVALID_HANDLE)
      {
      this->SetLastErrorText("CacheWideStringColumn: Attempted to read from invalid handle!");
      delete [] buffer;
      this->Internals->CurrentRow->SetValue(column, vtkVariant());
      return false;
      }
    }

  delete [] buffer;
  this->Internals->CurrentRow->SetValue(column, vtkVariant(outbuf.str()));
  return true;
}


// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheBinaryColumn(int column)
{
  SQLRETURN status;
  char *buffer;
  vtkStdString result;

  SQLSMALLINT nameLength;
  SQLSMALLINT columnType;
  SQLULEN columnSize;
  SQLSMALLINT columnScale;
  SQLSMALLINT nullable;
  SQLLEN indicator;
  SQLTCHAR namebuf[1024];

  status = SQLDescribeCol(this->Internals->Statement,
                          column + 1,
                          namebuf,
                          1024,
                          &nameLength,
                          &columnType,
                          &columnSize,
                          &columnScale,
                          &nullable);

  if (status != SQL_SUCCESS)
    {
    vtksys_ios::ostringstream errbuf;
    errbuf << "CacheBinaryColumn: Unable to describe column "
           << column << ": "
           << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
    this->Internals->CurrentRow->SetValue(column, vtkVariant());
    this->SetLastErrorText(errbuf.str().c_str());
    return false;
    }

  // If the data is smaller than 64k just read it in one
  // chunk. Otherwise read it in multiple passes.
  if (columnSize == 0)
    {
    columnSize = 256; // maybe it can't be determined
    }
  else if (columnSize > 65536)
    {
    columnSize = 65536; // read in 64k chunks
    }
  buffer = new char[columnSize];

  this->SetLastErrorText(NULL);

  vtksys_ios::ostringstream resultbuf;
  vtksys_ios::ostringstream outbuf;

  while (true)
    {
    status = SQLGetData(this->Internals->Statement,
                        column+1,
                        SQL_C_CHAR,
                        static_cast<SQLPOINTER>(buffer),
                        columnSize,
                        &indicator);
/*
    cerr << "once around the read loop for column " << column << ": status "
    << status << ", indicator " << indicator << "\n";
*/

    int bytesToWrite = 0;
    if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
      {
      if (status == SQL_NO_DATA)
        {
        // done reading!
        break;
        }
      else if (indicator == SQL_NULL_DATA)
        {
//        cerr << "Wide string value for column " << column << " is null\n";
        break;
        }
      // If we get to this point then there's data to read.
      if (indicator == SQL_NO_TOTAL)
        {
        bytesToWrite = columnSize;
        }
      else if (indicator > static_cast<SQLLEN>(columnSize))
        {
        bytesToWrite = columnSize;
        }
      else
        {
        bytesToWrite = indicator;
        }

      //      cerr << "Writing " << bytesToWrite << " characters in string column\n";
      
      if (status == SQL_SUCCESS_WITH_INFO)
        {
        // eat the null terminator
        bytesToWrite -= 1;
        }
      outbuf.write(buffer, bytesToWrite);
      if (status == SQL_SUCCESS)
        {
        // we retrieved everything in one pass
        break;
        }
      }
    else if (status == SQL_ERROR)
      {
      // there was some sort of error
      vtksys_ios::ostringstream errbuf;
      errbuf << "Error while reading binary column " << column << ": "
             << GetErrorMessage(SQL_HANDLE_STMT, this->Internals->Statement);
      this->SetLastErrorText(errbuf.str().c_str());
      cerr << errbuf.str() << "\n";
      delete [] buffer;
      this->Internals->CurrentRow->SetValue(column, vtkVariant());
      return false;
      }
    else if (status == SQL_INVALID_HANDLE)
      {
      this->SetLastErrorText("CacheWideStringColumn: Attempted to read from invalid handle!");
      delete [] buffer;
      this->Internals->CurrentRow->SetValue(column, vtkVariant());
      return false;
      }
    }

  delete [] buffer;
  this->Internals->CurrentRow->SetValue(column, vtkVariant(outbuf.str()));
  return true;
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheDecimalColumn(int column)
{
  this->Internals->CurrentRow->SetValue(column, vtkVariant());
  this->SetLastErrorText(NULL);
  return true;
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheNumericColumn(int column)
{
  this->Internals->CurrentRow->SetValue(column, vtkVariant());
  this->SetLastErrorText(NULL);
  return true;
}


// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheTimeColumn(int column)
{
  this->Internals->CurrentRow->SetValue(column, vtkVariant());
  this->SetLastErrorText(NULL);
  return true;
}


// ----------------------------------------------------------------------

bool
vtkODBCQuery::CacheIntervalColumn(int column)
{
  this->Internals->CurrentRow->SetValue(column, vtkVariant());
  this->SetLastErrorText(NULL);
  return true;
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, unsigned char value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, signed char value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, unsigned short value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, signed short value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, unsigned int value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, signed int value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, unsigned long value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, signed long value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, vtkTypeUInt64 value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, vtkTypeInt64 value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, float value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, double value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, const char *value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, const vtkStdString &value)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, const char *data, size_t length)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(data,
                                                                       length,
                                                                       false));
  return true;
}

// ----------------------------------------------------------------------

bool 
vtkODBCQuery::BindParameter(int index, const void *data, size_t length)
{
  this->Internals->SetBoundParameter(index, vtkBuildODBCBoundParameter(reinterpret_cast<const char *>(data),
                                                                       length,
                                                                       true));
  return true;
}

// ----------------------------------------------------------------------

bool
vtkODBCQuery::ClearParameterBindings()
{
  this->Internals->ClearBoundParameters();
  return true;
}



