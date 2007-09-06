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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkSQLiteQuery.h"

#include "vtkObjectFactory.h"
#include "vtkSQLiteDatabase.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtksqlite/vtk_sqlite3.h>

#include <assert.h>

#include <vtksys/ios/sstream>

#define BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

vtkCxxRevisionMacro(vtkSQLiteQuery, "1.2");
vtkStandardNewMacro(vtkSQLiteQuery);

// ----------------------------------------------------------------------
vtkSQLiteQuery::vtkSQLiteQuery() 
{
  this->Statement = NULL;
  this->InitialFetch = true;
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
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::Execute()
{

  if (this->Query == NULL)
    {
    vtkErrorMacro(<<"Cannot execute before a query has been set.");
    return false;
    }

  if (this->Statement != NULL)
    {
    int finalizeStatus = vtk_sqlite3_finalize(this->Statement);
    if (finalizeStatus != VTK_SQLITE_OK)
      {
      vtkWarningMacro(<<"Execute(): Finalize returned unexpected code "
                      << finalizeStatus);
      }
    this->Statement = NULL;
    }

  vtkSQLiteDatabase *dbContainer = 
    vtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != NULL);

  vtk_sqlite3 *db = dbContainer->SQLiteInstance;
  const char *unused_statement;

  int prepareStatus = vtk_sqlite3_prepare_v2(db, 
                                             this->Query,
                                             strlen(this->Query),
                                             &this->Statement,
                                             &unused_statement);

  if (prepareStatus != VTK_SQLITE_OK)
    {
    this->SetLastErrorText(vtk_sqlite3_errmsg(db));
    vtkDebugMacro(<<"Execute(): vtk_sqlite3_prepare_v2() failed with error message "
                  << this->GetLastErrorText());
    this->Active = false;
    return false;
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
      vtkSQLiteDatabase *dbContainer = 
        dynamic_cast<vtkSQLiteDatabase *>(this->Database);
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
      // XXX BLOB support has not been properly exercised yet.
      const char *blobData = reinterpret_cast<const char *>(vtk_sqlite3_column_text(this->Statement, column));

      return vtkVariant(vtkStdString(blobData));
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
  if (this->Database == NULL)
    {
    return "No database.";
    }
  else
    {
    return this->Database->GetLastErrorText();
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::HasError()
{
  if (this->Database == NULL)
    {
    return false;
    }
  if (this->Statement == NULL)
    {
    return false;
    }
  else
    {
    return (this->GetLastErrorText() != NULL);
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::BeginTransaction()
{
  if (this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot start a transaction.  One is already in progress.");
    return false;
    }

  vtkSQLiteDatabase *dbContainer = dynamic_cast<vtkSQLiteDatabase *>(this->Database);
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
  
  vtkSQLiteDatabase *dbContainer = 
    dynamic_cast<vtkSQLiteDatabase *>(this->Database);
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

  vtkSQLiteDatabase *dbContainer = 
    dynamic_cast<vtkSQLiteDatabase *>(this->Database);
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
