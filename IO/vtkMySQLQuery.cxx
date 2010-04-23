/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMySQLQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMySQLQuery.h"
#include "vtkMySQLDatabase.h"
#include "vtkMySQLDatabasePrivate.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <mysql.h>
#include <errmsg.h>


#if defined(WIN32)
# include <string.h>
# include <locale.h>
# define LOWERCASE_COMPARE _stricmp
#else
# include <strings.h>
# define LOWERCASE_COMPARE strcasecmp
#endif

#include <assert.h>

#include <vtksys/ios/sstream>
#include <vtksys/stl/vector>

/*
 * Bound Parameters and MySQL
 *
 * MySQL handles prepared statements using a MYSQL_STMT struct.
 * Parameters are bound to placeholders using a MYSQL_BIND struct.
 * The MYSQL_BIND contains a pointer to a data buffer.  That buffer
 * needs to be freed somehow when it's no longer needed.
 *
 * I'm going to handle this by using my own class
 * (vtkMySQLBoundParameter) to hold all the information the user
 * passes in.  At execution time, I'll take those parameters and
 * assemble the array of MYSQL_BIND objects that
 * mysql_stmt_bind_param() expects.  The vtkMySQLBoundParameter
 * instances will each own the buffers for their data.
 *
 * This is slightly inefficient in that it will generate
 * a few tiny little new[] requests.  If this ever becomes a problem,
 * we can allocate a fixed-size buffer (8 or 16 bytes) inside
 * vtkMySQLBoundParameter and use that for the data storage by
 * default.  That will still require special-case handling for blobs
 * and strings.
 *
 * The vtkMySQLQueryInternals class will handle the bookkeeping for
 * which parameters are and aren't bound at any given time.
 */


// ----------------------------------------------------------------------

class vtkMySQLBoundParameter
{
public:
  vtkMySQLBoundParameter() :
    IsNull(true), Data(NULL), BufferSize(0), DataLength(0), HasError(false)
    {
    }

  ~vtkMySQLBoundParameter()
    {
      delete [] this->Data;
    }

  void SetData(char *data, unsigned long size)
    {
      delete [] this->Data;
      this->BufferSize = size;
      this->Data = new char[size];
      memcpy(this->Data, data, size);
    }

  MYSQL_BIND BuildParameterStruct()
    {
      MYSQL_BIND output;
      output.buffer_type = this->DataType;
      output.buffer = this->Data;
      output.buffer_length = this->BufferSize;
      output.length = &(this->DataLength);
      output.is_null = &(this->IsNull);
      output.is_unsigned = this->IsUnsigned;
      output.error = NULL;
      return output;
    }

public:
  my_bool           IsNull;      // Is this parameter NULL?
  my_bool           IsUnsigned;  // For integer types, is it unsigned?
  char             *Data;        // Buffer holding actual data
  unsigned long     BufferSize;  // Buffer size
  unsigned long     DataLength;  // Size of the data in the buffer (must
                              // be less than or equal to BufferSize)
  my_bool           HasError;    // for the server to report truncation
  enum enum_field_types  DataType;    // MySQL data type for the contained data
};

// ----------------------------------------------------------------------

MYSQL_BIND BuildNullParameterStruct()
{
  MYSQL_BIND output;
  output.buffer_type = MYSQL_TYPE_NULL;
  return output;
}

// ----------------------------------------------------------------------

#define VTK_MYSQL_TYPENAME_MACRO(type,return_type) \
  enum enum_field_types vtkMySQLTypeName(type) \
  { return return_type; }

VTK_MYSQL_TYPENAME_MACRO(signed char, MYSQL_TYPE_TINY);
VTK_MYSQL_TYPENAME_MACRO(unsigned char, MYSQL_TYPE_TINY);
VTK_MYSQL_TYPENAME_MACRO(signed short, MYSQL_TYPE_SHORT);
VTK_MYSQL_TYPENAME_MACRO(unsigned short, MYSQL_TYPE_SHORT);
VTK_MYSQL_TYPENAME_MACRO(signed int, MYSQL_TYPE_LONG);
VTK_MYSQL_TYPENAME_MACRO(unsigned int, MYSQL_TYPE_LONG);
VTK_MYSQL_TYPENAME_MACRO(signed long, MYSQL_TYPE_LONG);
VTK_MYSQL_TYPENAME_MACRO(unsigned long, MYSQL_TYPE_LONG);
VTK_MYSQL_TYPENAME_MACRO(float, MYSQL_TYPE_FLOAT);
VTK_MYSQL_TYPENAME_MACRO(double, MYSQL_TYPE_DOUBLE);
VTK_MYSQL_TYPENAME_MACRO(vtkTypeInt64, MYSQL_TYPE_LONGLONG);
VTK_MYSQL_TYPENAME_MACRO(vtkTypeUInt64, MYSQL_TYPE_LONGLONG);
VTK_MYSQL_TYPENAME_MACRO(const char *, MYSQL_TYPE_STRING);
VTK_MYSQL_TYPENAME_MACRO(char *, MYSQL_TYPE_STRING);
VTK_MYSQL_TYPENAME_MACRO(void *, MYSQL_TYPE_BLOB);

template<typename T>
bool vtkMySQLIsTypeUnsigned(T)
{
  return false;
}
  
template<>
bool vtkMySQLIsTypeUnsigned<unsigned char>(unsigned char)
{
  return true;
}

template<>
bool vtkMySQLIsTypeUnsigned<unsigned short>(unsigned short)
{
  return true;
}

template<>
bool vtkMySQLIsTypeUnsigned<unsigned int>(unsigned int)
{
  return true;
}

template<>
bool vtkMySQLIsTypeUnsigned<unsigned long>(unsigned long)
{
  return true;
}

template<>
bool vtkMySQLIsTypeUnsigned<vtkTypeUInt64>(vtkTypeUInt64)
{
  return true;
}

// ----------------------------------------------------------------------

// Description:
// This function will build and populate a vtkMySQLBoundParameter
// struct.  The default implementation works for POD data types (char,
// int, long, etc).  I'll need to special-case strings and blobs.

template<typename T>
vtkMySQLBoundParameter *vtkBuildBoundParameter(T data_value)
{
  vtkMySQLBoundParameter *param = new vtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = vtkMySQLIsTypeUnsigned(data_value);
  param->DataType =   vtkMySQLTypeName(data_value);
  param->BufferSize = sizeof(T);
  param->DataLength = sizeof(T);
  param->SetData(new char[sizeof(T)], sizeof(T));
  memcpy(param->Data, &data_value, sizeof(T));

  return param;
}

// Description:
// Specialization of vtkBuildBoundParameter for NULL-terminated
// strings (i.e. CHAR and VARCHAR fields)

template<>
vtkMySQLBoundParameter *vtkBuildBoundParameter<const char *>(const char *data_value)
{
  vtkMySQLBoundParameter *param = new vtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = false; 
  param->DataType = MYSQL_TYPE_STRING;
  param->BufferSize = strlen(data_value);
  param->DataLength = strlen(data_value);
  param->SetData(new char[param->BufferSize], param->BufferSize);
  memcpy(param->Data, data_value, param->BufferSize);

  return param;
}

// Description:
// Alternate signature for vtkBuildBoundParameter to handle blobs

vtkMySQLBoundParameter *vtkBuildBoundParameter(const char *data,
                                               unsigned long length,
                                               bool is_blob)
{
  vtkMySQLBoundParameter *param = new vtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = false;
  param->BufferSize = length; 
  param->DataLength = length; 
  param->SetData(new char[length], length);
  memcpy(param->Data, data, param->BufferSize);

  if (is_blob)
    {
    param->DataType = MYSQL_TYPE_BLOB;
    }
  else
    {
    param->DataType = MYSQL_TYPE_STRING;
    }

  return param;
}
                                               
// ----------------------------------------------------------------------

class vtkMySQLQueryInternals
{
public:
  vtkMySQLQueryInternals();
  ~vtkMySQLQueryInternals();

  void FreeResult();
  void FreeStatement();
  void FreeUserParameterList();
  void FreeBoundParameters();
  bool SetQuery(const char *queryString, MYSQL *db, vtkStdString &error_message);
  bool SetBoundParameter(int index, vtkMySQLBoundParameter *param);
  bool BindParametersToStatement();

  // Description:
  // MySQL can only handle certain statements as prepared statements:
  // CALL, CREATE TABLE, DELETE, DO, INSERT, REPLACE, SELECT, SET,
  // UPDATE and some SHOW statements.  This function checks for those.
  // If we can't use a prepared statement then we have to do the query
  // the old-fashioned way.
  bool ValidPreparedStatementSQL(const char *query);

public:
  MYSQL_STMT      *Statement;
  MYSQL_RES       *Result;
  MYSQL_BIND      *BoundParameters;
  MYSQL_ROW        CurrentRow;
  unsigned long   *CurrentLengths;

  typedef vtksys_stl::vector<vtkMySQLBoundParameter *> ParameterList;
  ParameterList UserParameterList;
};

// ----------------------------------------------------------------------

vtkMySQLQueryInternals::vtkMySQLQueryInternals()
  : Statement(NULL), 
    Result(NULL), 
    BoundParameters(NULL), 
    CurrentLengths(NULL)
{
}

// ----------------------------------------------------------------------

vtkMySQLQueryInternals::~vtkMySQLQueryInternals()
{
  this->FreeResult();
  this->FreeStatement();
  this->FreeUserParameterList();
  this->FreeBoundParameters();
}

// ----------------------------------------------------------------------

void vtkMySQLQueryInternals::FreeResult()
{
  if (this->Result)
    {
    mysql_free_result(this->Result);
    this->Result = NULL;
    }
}

// ----------------------------------------------------------------------

void vtkMySQLQueryInternals::FreeStatement()
{
  if (this->Statement)
    {
    mysql_stmt_close(this->Statement);
    this->Statement = NULL;
    }
}

// ----------------------------------------------------------------------

bool vtkMySQLQueryInternals::SetQuery(const char *queryString, 
                                      MYSQL *db,
                                      vtkStdString &error_message)
{
  this->FreeStatement();
  this->FreeUserParameterList();
  this->FreeBoundParameters();

  if (this->ValidPreparedStatementSQL(queryString) == false)
    {
    return true; // we'll have to handle this query in immediate mode
    }

  this->Statement = mysql_stmt_init(db);
  if (this->Statement == NULL)
    {
    error_message = vtkStdString("vtkMySQLQuery: mysql_stmt_init returned out of memory error");
    return false;
    }
  
  int status = mysql_stmt_prepare(this->Statement,
                                  queryString,
                                  strlen(queryString));

  if (status == 0)
    {
    this->UserParameterList.resize(mysql_stmt_param_count(this->Statement), NULL);
    return true;
    }
  else
    {
    error_message = vtkStdString(mysql_stmt_error(this->Statement));
    return false;
    }
}

// ----------------------------------------------------------------------

void vtkMySQLQueryInternals::FreeUserParameterList()
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

void vtkMySQLQueryInternals::FreeBoundParameters()
{
  delete [] this->BoundParameters;
}

// ----------------------------------------------------------------------

bool vtkMySQLQueryInternals::SetBoundParameter(int index, vtkMySQLBoundParameter *param)
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

bool vtkMySQLQueryInternals::BindParametersToStatement()
{
  if (this->Statement == NULL)
    {
    vtkGenericWarningMacro(<<"BindParametersToStatement: No prepared statement available");
    return false;
    }
  
  this->FreeBoundParameters();
  unsigned long numParams = mysql_stmt_param_count(this->Statement);
  this->BoundParameters = new MYSQL_BIND[numParams];
  for (unsigned int i = 0; i < numParams; ++i)
    {
    if (this->UserParameterList[i])
      {
      this->BoundParameters[i] = this->UserParameterList[i]->BuildParameterStruct();
      }
    else
      {
      this->BoundParameters[i] = BuildNullParameterStruct();
      }
    }
  
  return mysql_stmt_bind_param(this->Statement, this->BoundParameters);
}

// ----------------------------------------------------------------------

bool vtkMySQLQueryInternals::ValidPreparedStatementSQL(const char *query)
{
  if (!LOWERCASE_COMPARE("call", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("create table", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("delete", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("do", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("insert", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("replace", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("select", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("set", query))
    {
    return true;
    }
  else if (!LOWERCASE_COMPARE("update", query))
    {
    return true;
    }
  return false;
}

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkMySQLQuery);

// ----------------------------------------------------------------------

vtkMySQLQuery::vtkMySQLQuery() 
{
  this->Internals = new vtkMySQLQueryInternals;
  this->InitialFetch = true;
  this->LastErrorText = NULL;
}

// ----------------------------------------------------------------------

vtkMySQLQuery::~vtkMySQLQuery()
{
  this->SetLastErrorText(NULL);
  delete this->Internals;
}

// ----------------------------------------------------------------------

void
vtkMySQLQuery::PrintSelf(ostream  &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
bool
vtkMySQLQuery::Execute()
{
  this->Active = false;

  if (this->Query == NULL)
    {
    vtkErrorMacro(<<"Cannot execute before a query has been set.");
    return false;
    }

  this->Internals->FreeResult();

  vtkMySQLDatabase *dbContainer = 
    static_cast<vtkMySQLDatabase *>(this->Database);
  assert(dbContainer != NULL);

  if (!dbContainer->IsOpen())
    {
    vtkErrorMacro(<<"Cannot execute query.  Database is closed.");
    return false;
    }

  vtkDebugMacro(<<"Execute(): Query ready to execute.");

  if (this->Query != NULL && this->Internals->Statement == NULL)
    {
    MYSQL *db = dbContainer->Private->Connection;
    assert(db != NULL);

    int result = mysql_query(db, this->Query);
    if (result == 0)
      {
      // The query succeeded.
      this->SetLastErrorText(NULL);
      this->Active = true;
      this->Internals->Result = mysql_store_result(db);
      return true;
      }
    else
      {
      this->Active = false;
      this->SetLastErrorText(mysql_error(db));
      vtkErrorMacro(<<"Query returned an error: "
                    << this->GetLastErrorText());
      return false;
      }
    }
  else
    {
    vtkDebugMacro(<<"Binding parameters immediately prior to execution.");
    bool bindStatus = this->Internals->BindParametersToStatement();
    if (!bindStatus)
      {
      this->SetLastErrorText(mysql_stmt_error(this->Internals->Statement));
      vtkErrorMacro(<<"Error binding parameters: "
                    <<this->GetLastErrorText());
      return false;
      }

    int result = mysql_stmt_execute(this->Internals->Statement);
    if (result == 0)
      {
      // The query succeeded.
      this->SetLastErrorText(NULL);
      this->Active = true;
      this->Internals->Result = mysql_stmt_result_metadata(this->Internals->Statement);
      return true;
      }
    else
      {
      this->Active = false;
      this->SetLastErrorText(mysql_stmt_error(this->Internals->Statement));
      vtkErrorMacro(<<"Query returned an error: "
                    << this->GetLastErrorText());
      return false;
      }
    }
}

// ----------------------------------------------------------------------

int
vtkMySQLQuery::GetNumberOfFields()
{
  if (! this->Active)
    {
    vtkErrorMacro(<<"GetNumberOfFields(): Query is not active!");
    return 0;
    }
  else
    {
    return static_cast<int>(mysql_num_fields(this->Internals->Result));
    }
}

// ----------------------------------------------------------------------

const char *
vtkMySQLQuery::GetFieldName(int column)
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
    MYSQL_FIELD *field = mysql_fetch_field_direct(this->Internals->Result, column);
    if (field == NULL)
      {
      vtkErrorMacro(<<"GetFieldName(): MySQL returned null field for column "
                    << column );
      return NULL;
      }
    else
      {
      return field->name;
      }
    }
}

// ----------------------------------------------------------------------

int
vtkMySQLQuery::GetFieldType(int column)
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
    vtkMySQLDatabase *dbContainer = 
      static_cast<vtkMySQLDatabase *>(this->Database);
    assert(dbContainer != NULL);
    if (!dbContainer->IsOpen())
      {
      vtkErrorMacro(<<"Cannot get field type.  Database is closed.");
      return VTK_VOID;
      }
    
    MYSQL *db = dbContainer->Private->Connection;
    assert(db != NULL);

    MYSQL_FIELD *field = mysql_fetch_field_direct(this->Internals->Result, 
                                                  column);
    if (field == NULL)
      {
      vtkErrorMacro(<<"GetFieldType(): MySQL returned null field for column "
                    << column );
      return -1;
      }
    else
      {

      switch (field->type)
      {
      case MYSQL_TYPE_ENUM:
      case MYSQL_TYPE_TINY:
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_YEAR:
        return VTK_INT;

      case MYSQL_TYPE_SHORT:
        return VTK_SHORT;

      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_LONGLONG:
        return VTK_LONG;

      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_NEWDATE:
        return VTK_STRING;  // Just return the raw string.

#if MYSQL_VERSION_ID >= 50000
      case MYSQL_TYPE_BIT:
        return VTK_BIT;
#endif

      case MYSQL_TYPE_FLOAT:
        return VTK_FLOAT;

      case MYSQL_TYPE_DOUBLE:
      case MYSQL_TYPE_DECIMAL:
#if MYSQL_VERSION_ID >= 50000
      case MYSQL_TYPE_NEWDECIMAL:
#endif
        return VTK_DOUBLE;
        
      case MYSQL_TYPE_NULL:
        return VTK_VOID;
        
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
        return VTK_STRING; // MySQL treats text fields as blobs

      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
#if MYSQL_VERSION_ID >= 50000
      case MYSQL_TYPE_VARCHAR:
#endif
        return VTK_STRING;

      case MYSQL_TYPE_SET:
      case MYSQL_TYPE_GEOMETRY:
      default:
      {
      vtkErrorMacro( <<"GetFieldType(): Unknown data type " 
                     << field->type );
      return VTK_VOID;
      }
      }
      }
    }
}

// ----------------------------------------------------------------------

bool
vtkMySQLQuery::NextRow()
{
  if (! this->IsActive())
    {
    vtkErrorMacro(<<"NextRow(): Query is not active!");
    return false;
    }

  MYSQL_ROW row = mysql_fetch_row(this->Internals->Result);
  this->Internals->CurrentRow = row;
  this->Internals->CurrentLengths = mysql_fetch_lengths(this->Internals->Result);

  if (!row)
    {
    // A null row will come back in one of two situations.  The first
    // is when there's an error, in which case mysql_errno will return
    // some nonzero error code.  The second is when there are no more
    // rows to fetch.  Discriminate between the two by checking the
    // errno.
    this->Active = false;
    vtkMySQLDatabase *dbContainer = 
      static_cast<vtkMySQLDatabase *>(this->Database);
    assert(dbContainer != NULL);
    if (!dbContainer->IsOpen())
      {
      vtkErrorMacro(<<"Cannot get field type.  Database is closed.");
      this->SetLastErrorText("Database is closed.");
      return VTK_VOID;
      }
    MYSQL *db = dbContainer->Private->Connection;
    assert(db != NULL);


    if (mysql_errno(db) != 0)
      {
      this->SetLastErrorText(mysql_error(db));
      vtkErrorMacro(<<"NextRow(): MySQL returned error message "
                    << this->GetLastErrorText());
      }
    else
      {
      // Nothing's wrong.  We're just out of results.
      this->SetLastErrorText(NULL);
      }

    return false;
    }
  else
    {
    this->SetLastErrorText(NULL);
    return true;
    }
}

// ----------------------------------------------------------------------

vtkVariant
vtkMySQLQuery::DataValue(vtkIdType column)
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
    assert(this->Internals->CurrentRow);
    vtkVariant result;

    // Initialize base as a VTK_VOID value... only populate with
    // data when a column value is non-NULL.
    vtkVariant base;
    if ( this->Internals->CurrentRow[column] )
      {
      // Make a string holding the data, including possible embedded null characters.
      vtkStdString s( this->Internals->CurrentRow[column],
        static_cast<size_t>(this->Internals->CurrentLengths[column]) );
      base = vtkVariant( s );
      }

    // It would be a royal pain to try to convert the string to each
    // different value in turn.  Fortunately, there is already code in
    // vtkVariant to do exactly that using the C++ standard library
    // sstream.  We'll exploit that.
    switch (this->GetFieldType(column))
      {
      case VTK_INT:
      case VTK_SHORT:
      case VTK_BIT:
        return vtkVariant(base.ToInt());

      case VTK_LONG:
      case VTK_UNSIGNED_LONG:
        return vtkVariant(base.ToLong());

      case VTK_FLOAT:
        return vtkVariant(base.ToFloat());

      case VTK_DOUBLE:
        return vtkVariant(base.ToDouble());
        
      case VTK_STRING:
        return base; // it's already a string

      case VTK_VOID:
        return vtkVariant();

      default:
      {
      vtkWarningMacro(<<"Unhandled type " << this->GetFieldType(column)
                      <<" in DataValue().");
      return vtkVariant();
      }
      }
    }
}

  
// ----------------------------------------------------------------------

const char *
vtkMySQLQuery::GetLastErrorText()
{
  return this->LastErrorText; 
}

// ----------------------------------------------------------------------

bool
vtkMySQLQuery::HasError()
{
  return (this->GetLastErrorText() != NULL);
}

vtkStdString vtkMySQLQuery::EscapeString( vtkStdString src, bool addSurroundingQuotes )
{
  vtkStdString dst;
  vtkMySQLDatabase* dbContainer =
    static_cast<vtkMySQLDatabase*>( this->Database );
  assert( dbContainer != NULL );

  MYSQL* db;
  if ( ( ! dbContainer->IsOpen() ) || ! ( db = dbContainer->Private->Connection ) )
    { // fall back to superclass implementation
    dst = this->Superclass::EscapeString( src, addSurroundingQuotes );
    return dst;
    }

  unsigned long ssz = src.size();
  char* dstarr = new char[2 * ssz + (addSurroundingQuotes ? 3 : 1)];
  char* end = dstarr;
  if ( addSurroundingQuotes )
    {
    * ( end ++ ) = '\'';
    }
  end += mysql_real_escape_string( db, end, src.c_str(), ssz );
  if ( addSurroundingQuotes )
    {
    * ( end ++ ) = '\'';
    * ( end ++ ) = '\0';
    }
  dst = dstarr;
  delete [] dstarr;
  return dst;
}

// ----------------------------------------------------------------------

bool
vtkMySQLQuery::SetQuery(const char *newQuery)
{
  vtkDebugMacro(<< this->GetClassName() 
                << " (" << this << "): setting Query to "  
                << (newQuery?newQuery:"(null)") ); 
  
  if (this->Query == NULL && newQuery == NULL)
    {
    return true;
    }

  if (this->Query && newQuery && (!strcmp(this->Query, newQuery)))
    {
    return true; // we've already got that query
    }

  if (this->Query)
    {
    delete [] this->Query;
    }

  if (newQuery)
    {
    // Keep a local copy of the query - this is from vtkSetGet.h
    size_t n = strlen(newQuery) + 1;                
    char *cp1 =  new char[n]; 
    const char *cp2 = (newQuery); 
    this->Query = cp1; 
    do { *cp1++ = *cp2++; } while ( --n ); 
    } 
   else 
    { 
    this->Query = NULL; 
    } 

  // If we get to this point the query has changed.  We need to
  // finalize the already-prepared statement if one exists and then
  // prepare a new statement.
  this->Active = false;

  vtkMySQLDatabase *dbContainer = 
    static_cast<vtkMySQLDatabase *>(this->Database);
  if (!dbContainer)
    {
    vtkErrorMacro(<< "SetQuery: No database connection set!  This usually happens if you have instantiated vtkMySQLQuery directly.  Don't do that.  Call vtkSQLDatabase::GetQueryInstance instead.");
    return false;
    }

  MYSQL *db = dbContainer->Private->Connection;
  assert(db != NULL);

  vtkStdString errorMessage;
  bool success = this->Internals->SetQuery(this->Query, db, errorMessage);
  if (!success)
    {
    this->SetLastErrorText(errorMessage.c_str());
    vtkErrorMacro(<<"SetQuery: Error while preparing statement: "
                  <<errorMessage.c_str());
    }
  return success;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, unsigned char value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, signed char value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, unsigned short value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, signed short value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, unsigned int value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, signed int value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, unsigned long value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, signed long value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, vtkTypeUInt64 value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, vtkTypeInt64 value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, float value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}


// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, double value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, const char *value)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, const vtkStdString &value)
{
  return this->BindParameter(index, value.c_str());
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, const char *data, size_t length)
{
  this->Internals->SetBoundParameter(index, vtkBuildBoundParameter(data,
                                                                   length,
                                                                   false));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::BindParameter(int index, const void *data, size_t length)
{
  this->Internals->SetBoundParameter(index,
                                     vtkBuildBoundParameter(static_cast<const char *>(data),
                                                            length,
                                                            true));
  return true;
}

// ----------------------------------------------------------------------

bool vtkMySQLQuery::ClearParameterBindings()
{
  this->Internals->FreeBoundParameters();
  return true;
}


                                                        
                                                                   
