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
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>

#include <mysql.h>

#include <assert.h>

#include <vtksys/ios/sstream>

// ----------------------------------------------------------------------

class vtkMySQLQueryInternals
{
public:
  vtkMySQLQueryInternals()
    { 
      this->Result = NULL;
      this->CurrentRow = NULL;
    }

  ~vtkMySQLQueryInternals()
    {
      this->FreeResult();
    }

  void FreeResult()
    {
      if (this->Result)
        {
        mysql_free_result(this->Result);
        this->Result = NULL;
        }
    }

public:
  MYSQL_RES *Result;
  MYSQL_ROW CurrentRow;
};

// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkMySQLQuery, "1.2");
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

  MYSQL *db = dbContainer->Connection;
  assert(db != NULL);

  vtkDebugMacro(<<"Execute(): Query ready to execute.");

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
    
    MYSQL *db = dbContainer->Connection;
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
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_ENUM:
      case MYSQL_TYPE_TINY:
      case MYSQL_TYPE_INT24:
#if MYSQL_VERSION >= 50000
      case MYSQL_TYPE_NEWDECIMAL:
#endif
        return VTK_INT;

      case MYSQL_TYPE_SHORT:
        return VTK_SHORT;

      case MYSQL_TYPE_LONG:
        return VTK_LONG;

      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_LONGLONG:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_NEWDATE:
        return VTK_UNSIGNED_LONG;  // vtkTimePoint uses longs

#if MYSQL_VERSION >= 50000
      case MYSQL_TYPE_BIT:
        return VTK_BIT;
#endif

      case MYSQL_TYPE_FLOAT:
        return VTK_FLOAT;

      case MYSQL_TYPE_DOUBLE:
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
#if MYSQL_VERSION >= 50000
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
    MYSQL *db = dbContainer->Connection;
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

    // It would be a royal pain to try to convert the string to each
    // different value in turn.  Fortunately, there is already code in
    // vtkVariant to do exactly that using the C++ standard library
    // sstream.  We'll exploit that.
    vtkVariant base(this->Internals->CurrentRow[column]);

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
