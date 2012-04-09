/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteQuery.cxx

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
#include "vtkSQLiteQuery.h"

#include "vtkObjectFactory.h"
#include "vtkSQLiteDatabase.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtksqlite/vtk_sqlite3.h>

#include <assert.h>

#include <vtksys/ios/sstream>
#include <vtksys/stl/vector>

#define BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

vtkStandardNewMacro(vtkSQLiteQuery);

// ----------------------------------------------------------------------
vtkSQLiteQuery::vtkSQLiteQuery()
{
  this->Statement = NULL;
  this->InitialFetch = true;
  this->InitialFetchResult=VTK_SQLITE_DONE;
  this->LastErrorText = NULL;
  this->TransactionInProgress = false;
}

// ----------------------------------------------------------------------
vtkSQLiteQuery::~vtkSQLiteQuery()
{
  this->SetLastErrorText(NULL);
  if (this->TransactionInProgress)
    {
    this->RollbackTransaction();
    }

  if (this->Statement != NULL)
    {
    if (this->Database != NULL)
      {
      vtk_sqlite3_finalize(this->Statement);
      this->Statement = NULL;
      }
    }
}

// ----------------------------------------------------------------------
void vtkSQLiteQuery::PrintSelf(ostream  &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Statement: ";
  if (this->Statement)
    {
    os << this->Statement << "\n";
    }
  else
    {
    os << "(null)" << "\n";
    }
  os << indent << "InitialFetch: " << this->InitialFetch << "\n";
  os << indent << "InitialFetchResult: " << this->InitialFetchResult << "\n";
  os << indent << "TransactionInProgress: " << this->TransactionInProgress
     << "\n";
  os << indent << "LastErrorText: "
    << (this->LastErrorText ? this->LastErrorText : "(null)") << endl;
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::SetQuery(const char *newQuery)
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
  if (this->Statement)
    {
    vtkDebugMacro(<<"Finalizing old statement");
    int finalizeStatus = vtk_sqlite3_finalize(this->Statement);
    if (finalizeStatus != VTK_SQLITE_OK)
      {
      vtkWarningMacro(<<"SetQuery(): Finalize returned unexpected code "
                      << finalizeStatus);
      }
    this->Statement = NULL;
    }
  
  if (this->Query)
    {
    vtkSQLiteDatabase *dbContainer = 
      vtkSQLiteDatabase::SafeDownCast(this->Database);

    if (dbContainer == NULL)
      {
      vtkErrorMacro(<<"This should never happen: SetQuery() called when there is no underlying database.  You probably instantiated vtkSQLiteQuery directly instead of calling vtkSQLDatabase::GetInstance().  This also happens during TestSetGet in the CDash testing.");
      return false;
      }
    
    vtk_sqlite3 *db = dbContainer->SQLiteInstance;
    const char *unused_statement;
    
    int prepareStatus = vtk_sqlite3_prepare_v2(db, 
                                               this->Query,
                                               static_cast<int>(strlen(this->Query)),
                                               &this->Statement,
                                               &unused_statement);
    
    if (prepareStatus != VTK_SQLITE_OK)
      {
      this->SetLastErrorText(vtk_sqlite3_errmsg(db));
      vtkWarningMacro(<<"SetQuery(): vtk_sqlite3_prepare_v2() failed with error message "
                    << this->GetLastErrorText()
                    << " on statement: '"
                    << this->Query << "'");
      this->Active = false;
      return false;
      }
    } // Done preparing new statement
  
  this->Modified(); 
  return true;
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::Execute()
{

  if (this->Query == NULL)
    {
    vtkErrorMacro(<<"Cannot execute before a query has been set.");
    return false;
    }

  if (this->Statement == NULL)
    {
    vtkErrorMacro(<<"Execute(): Query is not null but prepared statement is.  There may have been an error during SetQuery().");
    this->Active = false;
    return false;
    }
  else
    {
    vtk_sqlite3_reset(this->Statement);
    }

  vtkDebugMacro(<<"Execute(): Query ready to execute.");

  this->InitialFetch = true;
  int result = vtk_sqlite3_step(this->Statement);
  this->InitialFetchResult = result;

  if (result == VTK_SQLITE_DONE)
    {
    this->SetLastErrorText(NULL);
    this->Active = true;
    return true;
    }
  else if (result != VTK_SQLITE_ROW)
    {
    vtkSQLiteDatabase *dbContainer = 
      vtkSQLiteDatabase::SafeDownCast(this->Database);
    assert(dbContainer != NULL);
    
    vtk_sqlite3 *db = dbContainer->SQLiteInstance;

    this->SetLastErrorText(vtk_sqlite3_errmsg(db));
    vtkDebugMacro(<< "Execute(): vtk_sqlite3_step() returned error message "
                  << this->GetLastErrorText());
    this->Active = false;
    return false;
    }

  this->SetLastErrorText(NULL);
  this->Active = true;
  return true;
}

// ----------------------------------------------------------------------
int vtkSQLiteQuery::GetNumberOfFields()
{
  if (! this->Active)
    {
    vtkErrorMacro(<<"GetNumberOfFields(): Query is not active!");
    return 0;
    }
  else
    {
    return vtk_sqlite3_column_count(this->Statement);
    }
}

// ----------------------------------------------------------------------
const char * vtkSQLiteQuery::GetFieldName(int column)
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
    return vtk_sqlite3_column_name(this->Statement, column);
    }
}

// ----------------------------------------------------------------------
int vtkSQLiteQuery::GetFieldType(int column)
{
  if (! this->Active)
    {
    vtkErrorMacro(<<"GetFieldType(): Query is not active!");
    return -1;
    }
  else if (column < 0 || column >= this->GetNumberOfFields())
    {
    vtkErrorMacro(<<"GetFieldType(): Illegal field index " 
                  << column);
    return -1;
    }
  else
    {
    switch (vtk_sqlite3_column_type(this->Statement, column))
      {
      case VTK_SQLITE_INTEGER:
        return VTK_INT; 
      case VTK_SQLITE_FLOAT:
        return VTK_FLOAT;
      case VTK_SQLITE_TEXT:
        return VTK_STRING;
      case VTK_SQLITE_BLOB:
        return VTK_STRING; // until we have a BLOB type of our own
      case VTK_SQLITE_NULL:
        return VTK_VOID; // ??? what makes sense here?
      default:
      {
      vtkErrorMacro(<<"GetFieldType(): Unknown data type " 
                    << vtk_sqlite3_column_type(this->Statement, column)
                    <<" from SQLite.");
      return VTK_VOID;
      }
      }
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::NextRow()
{
  if (! this->IsActive())
    {
    vtkErrorMacro(<<"NextRow(): Query is not active!");
    return false;
    }

  if (this->InitialFetch)
    {
    vtkDebugMacro(<<"NextRow(): Initial fetch being handled.");
    this->InitialFetch = false;
    if (this->InitialFetchResult == VTK_SQLITE_DONE)
      {
      return false;
      }
    else
      {
      return true;
      }
    }
  else
    {
    int result = vtk_sqlite3_step(this->Statement);
    if (result == VTK_SQLITE_DONE)
      {
      return false;
      }
    else if (result == VTK_SQLITE_ROW)
      {
      return true;
      }
    else
      {
      vtkSQLiteDatabase *dbContainer = vtkSQLiteDatabase::SafeDownCast( this->Database );
      assert(dbContainer != NULL);
      vtk_sqlite3 *db = dbContainer->SQLiteInstance;
      this->SetLastErrorText(vtk_sqlite3_errmsg(db));
      vtkErrorMacro(<<"NextRow(): Database returned error code " 
                    << result << " with the following message: "
                    << this->GetLastErrorText());
      this->Active = false;
      return false;
      }
    }
}

// ----------------------------------------------------------------------
vtkVariant vtkSQLiteQuery::DataValue(vtkIdType column)
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
    switch (vtk_sqlite3_column_type(this->Statement, column))
      {
      case VTK_SQLITE_INTEGER:
        return vtkVariant(vtk_sqlite3_column_int(this->Statement, column));

      case VTK_SQLITE_FLOAT:
        return vtkVariant(vtk_sqlite3_column_double(this->Statement, column));

      case VTK_SQLITE_TEXT:
      {
      vtksys_ios::ostringstream str;
      str << vtk_sqlite3_column_text(this->Statement, column);
      return vtkVariant(vtkStdString(str.str()));
      }

      case VTK_SQLITE_BLOB:
      {
      // This is a hack ... by passing the BLOB to vtkStdString with an explicit
      // byte count, we ensure that the string will store all of the BLOB's bytes,
      // even if there are NULL values.

      return vtkVariant(vtkStdString(
        static_cast<const char*>(vtk_sqlite3_column_blob(this->Statement, column)),
        vtk_sqlite3_column_bytes(this->Statement, column)));
      }
      
      case VTK_SQLITE_NULL:
      default:
        return vtkVariant();
      }
    }
}

// ----------------------------------------------------------------------
const char * vtkSQLiteQuery::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::HasError()
{
  return (this->GetLastErrorText() != NULL);
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::BeginTransaction()
{
  if (this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot start a transaction.  One is already in progress.");
    return false;
    }

  vtkSQLiteDatabase *dbContainer = vtkSQLiteDatabase::SafeDownCast( this->Database );
  assert(dbContainer != NULL);

  vtk_sqlite3 *db = dbContainer->SQLiteInstance;
  char *errorMessage = NULL;
  int result = vtk_sqlite3_exec(db, BEGIN_TRANSACTION, NULL, NULL, &errorMessage);

  if (result == VTK_SQLITE_OK)
    {
    this->TransactionInProgress = true;
    this->SetLastErrorText(NULL);
    vtkDebugMacro(<<"BeginTransaction() succeeded.");
    return true;
    }
  else
    {
    vtkErrorMacro(<<"BeginTransaction(): sqlite3_exec returned unexpected result code " << result);
    if (errorMessage)
      {
      vtkErrorMacro(<< " and error message " << errorMessage);
      }
    this->TransactionInProgress = false;
    return false;
    }
}
 
// ----------------------------------------------------------------------
bool vtkSQLiteQuery::CommitTransaction()
{
  if (this->Statement)
    {
    vtk_sqlite3_finalize(this->Statement);
    this->Statement = NULL;
    }

  if (!this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot commit.  There is no transaction in progress.");
    return false;
    }
  
  vtkSQLiteDatabase *dbContainer = vtkSQLiteDatabase::SafeDownCast( this->Database );
  assert(dbContainer != NULL);
  vtk_sqlite3 *db = dbContainer->SQLiteInstance;
  char *errorMessage = NULL;
  int result = vtk_sqlite3_exec(db, COMMIT_TRANSACTION, NULL, NULL, &errorMessage);

  if (result == VTK_SQLITE_OK)
    {
    this->TransactionInProgress = false;
    this->SetLastErrorText(NULL);
    vtkDebugMacro(<<"CommitTransaction() succeeded.");
    return true;
    }
  else
    {
    vtkErrorMacro(<<"CommitTransaction(): sqlite3_exec returned unexpected result code " << result);
    if (errorMessage)
      {
      this->SetLastErrorText(errorMessage);
      vtkErrorMacro(<< " and error message " << errorMessage);
      }
    assert(1==0);
    return false;
    }

}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::RollbackTransaction()
{
  if (!this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot rollback.  There is no transaction in progress.");
    return false;
    }

  vtkSQLiteDatabase *dbContainer = vtkSQLiteDatabase::SafeDownCast( this->Database );
  assert(dbContainer != NULL);
  vtk_sqlite3 *db = dbContainer->SQLiteInstance;
  char *errorMessage = NULL;
  int result = vtk_sqlite3_exec(db, ROLLBACK_TRANSACTION, NULL, NULL, &errorMessage);

  if (result == VTK_SQLITE_OK)
    {
    this->TransactionInProgress = false;
    this->SetLastErrorText(NULL);
    vtkDebugMacro(<<"RollbackTransaction() succeeded.");
    return true;
    }
  else
    {
    vtkErrorMacro(<<"RollbackTransaction(): sqlite3_exec returned unexpected result code " << result);
    if (errorMessage)
      {
      this->SetLastErrorText(errorMessage);
      vtkErrorMacro(<< " and error message " << errorMessage);
      }
    return false;
    }
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, unsigned char value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, signed char value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, unsigned short value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, short value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, unsigned int value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, int value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, unsigned long value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, long value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, vtkTypeUInt64 value)
{
  return this->BindInt64Parameter(index, static_cast<vtkTypeInt64>(value));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, vtkTypeInt64 value)
{
  return this->BindInt64Parameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, float value)
{
  return this->BindDoubleParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, double value)
{
  return this->BindDoubleParameter(index, value);
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const char *value)
{
  return this->BindParameter(index, value, strlen(value));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const char *data, size_t length)
{
  return this->BindStringParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const vtkStdString &value)
{
  return this->BindParameter(index, value.c_str());
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const void *data, size_t length)
{
  return this->BindBlobParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindIntegerParameter(int index, int value)
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }
  
  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }
  int status = vtk_sqlite3_bind_int(this->Statement, index+1, value);

  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_bind_int returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<errormessage.str().c_str());
    return false;
    }
  return true;
}


// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindInt64Parameter(int index, vtkTypeInt64 value)
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }

  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }
  int status = vtk_sqlite3_bind_int(this->Statement, index+1, static_cast<vtk_sqlite_int64>(value));

  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_bind_int64 returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<this->GetLastErrorText());
    return false;
    }
  return true;
}


// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindDoubleParameter(int index, double value)
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }
  
  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }

  int status = vtk_sqlite3_bind_double(this->Statement, index+1, value);

  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_bind_double returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<this->GetLastErrorText());
    return false;
    }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindStringParameter(int index, const char *value, int length)
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }

  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }
  
  int status = vtk_sqlite3_bind_text(this->Statement, index+1, value, length, VTK_SQLITE_TRANSIENT);

  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_bind_text returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<this->GetLastErrorText());
    return false;
    }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindBlobParameter(int index, const void *data, int length)
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }

  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }
  
  int status = 
    vtk_sqlite3_bind_blob(this->Statement, 
                          index+1, 
                          data, 
                          length, 
                          VTK_SQLITE_TRANSIENT);

  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_bind_blob returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<this->GetLastErrorText());
    return false;
    }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::ClearParameterBindings()
{
  if (!this->Statement)
    {
    vtkErrorMacro(<<"No statement available.  Did you forget to call SetQuery?");
    return false;
    }

  if (this->Active)
    {
    this->Active = false;
    vtk_sqlite3_reset(this->Statement);
    }

  int status = vtk_sqlite3_clear_bindings(this->Statement);
  
  if (status != VTK_SQLITE_OK)
    {
    vtksys_ios::ostringstream errormessage;
    errormessage << "sqlite_clear_bindings returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<<this->GetLastErrorText());
    return false;
    }
  return true;
}
    
// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, vtkVariant value)
{
  return this->Superclass::BindParameter(index, value);
}
