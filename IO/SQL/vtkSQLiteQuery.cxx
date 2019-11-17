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
#include "vtkSQLiteDatabaseInternals.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtk_sqlite.h"

#include <cassert>

#include <sstream>
#include <vector>

#define BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

class vtkSQLiteQuery::Priv
{
public:
  sqlite3_stmt* Statement;
};

vtkStandardNewMacro(vtkSQLiteQuery);

// ----------------------------------------------------------------------
vtkSQLiteQuery::vtkSQLiteQuery()
{
  this->Private = new Priv;
  this->Private->Statement = nullptr;
  this->InitialFetch = true;
  this->InitialFetchResult = SQLITE_DONE;
  this->LastErrorText = nullptr;
  this->TransactionInProgress = false;
}

// ----------------------------------------------------------------------
vtkSQLiteQuery::~vtkSQLiteQuery()
{
  this->SetLastErrorText(nullptr);
  if (this->TransactionInProgress)
  {
    this->RollbackTransaction();
  }

  if (this->Private->Statement != nullptr)
  {
    if (this->Database != nullptr)
    {
      sqlite3_finalize(this->Private->Statement);
      this->Private->Statement = nullptr;
    }
  }
  delete this->Private;
}

// ----------------------------------------------------------------------
void vtkSQLiteQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Statement: ";
  if (this->Private->Statement)
  {
    os << this->Private->Statement << "\n";
  }
  else
  {
    os << "(null)"
       << "\n";
  }
  os << indent << "InitialFetch: " << this->InitialFetch << "\n";
  os << indent << "InitialFetchResult: " << this->InitialFetchResult << "\n";
  os << indent << "TransactionInProgress: " << this->TransactionInProgress << "\n";
  os << indent << "LastErrorText: " << (this->LastErrorText ? this->LastErrorText : "(null)")
     << endl;
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::SetQuery(const char* newQuery)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Query to "
                << (newQuery ? newQuery : "(null)"));

  if (this->Query == nullptr && newQuery == nullptr)
  {
    return true;
  }

  if (this->Query && newQuery && (!strcmp(this->Query, newQuery)))
  {
    return true; // we've already got that query
  }

  delete[] this->Query;

  if (newQuery)
  {
    // Keep a local copy of the query - this is from vtkSetGet.h
    size_t n = strlen(newQuery) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (newQuery);
    this->Query = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->Query = nullptr;
  }

  // If we get to this point the query has changed.  We need to
  // finalize the already-prepared statement if one exists and then
  // prepare a new statement.
  if (this->Private->Statement)
  {
    vtkDebugMacro(<< "Finalizing old statement");
    int finalizeStatus = sqlite3_finalize(this->Private->Statement);
    if (finalizeStatus != SQLITE_OK)
    {
      vtkWarningMacro(<< "SetQuery(): Finalize returned unexpected code " << finalizeStatus);
    }
    this->Private->Statement = nullptr;
  }

  if (this->Query)
  {
    vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);

    if (dbContainer == nullptr)
    {
      vtkErrorMacro(<< "This should never happen: SetQuery() called when there is no underlying "
                       "database.  You probably instantiated vtkSQLiteQuery directly instead of "
                       "calling vtkSQLDatabase::GetInstance().  This also happens during "
                       "TestSetGet in the CDash testing.");
      return false;
    }

    sqlite3* db = dbContainer->Internal->SQLiteInstance;
    const char* unused_statement;

    int prepareStatus = sqlite3_prepare_v2(db, this->Query, static_cast<int>(strlen(this->Query)),
      &this->Private->Statement, &unused_statement);

    if (prepareStatus != SQLITE_OK)
    {
      this->SetLastErrorText(sqlite3_errmsg(db));
      vtkWarningMacro(<< "SetQuery(): sqlite3_prepare_v2() failed with error message "
                      << this->GetLastErrorText() << " on statement: '" << this->Query << "'");
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

  if (this->Query == nullptr)
  {
    vtkErrorMacro(<< "Cannot execute before a query has been set.");
    return false;
  }

  if (this->Private->Statement == nullptr)
  {
    vtkErrorMacro(<< "Execute(): Query is not null but prepared statement is.  There may have been "
                     "an error during SetQuery().");
    this->Active = false;
    return false;
  }
  else
  {
    sqlite3_reset(this->Private->Statement);
  }

  vtkDebugMacro(<< "Execute(): Query ready to execute.");

  this->InitialFetch = true;
  int result = sqlite3_step(this->Private->Statement);
  this->InitialFetchResult = result;

  if (result == SQLITE_DONE)
  {
    this->SetLastErrorText(nullptr);
    this->Active = true;
    return true;
  }
  else if (result != SQLITE_ROW)
  {
    vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);
    assert(dbContainer != nullptr);

    sqlite3* db = dbContainer->Internal->SQLiteInstance;

    this->SetLastErrorText(sqlite3_errmsg(db));
    vtkDebugMacro(<< "Execute(): sqlite3_step() returned error message "
                  << this->GetLastErrorText());
    this->Active = false;
    return false;
  }

  this->SetLastErrorText(nullptr);
  this->Active = true;
  return true;
}

// ----------------------------------------------------------------------
int vtkSQLiteQuery::GetNumberOfFields()
{
  if (!this->Active)
  {
    vtkErrorMacro(<< "GetNumberOfFields(): Query is not active!");
    return 0;
  }
  else
  {
    return sqlite3_column_count(this->Private->Statement);
  }
}

// ----------------------------------------------------------------------
const char* vtkSQLiteQuery::GetFieldName(int column)
{
  if (!this->Active)
  {
    vtkErrorMacro(<< "GetFieldName(): Query is not active!");
    return nullptr;
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    vtkErrorMacro(<< "GetFieldName(): Illegal field index " << column);
    return nullptr;
  }
  else
  {
    return sqlite3_column_name(this->Private->Statement, column);
  }
}

// ----------------------------------------------------------------------
int vtkSQLiteQuery::GetFieldType(int column)
{
  if (!this->Active)
  {
    vtkErrorMacro(<< "GetFieldType(): Query is not active!");
    return -1;
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    vtkErrorMacro(<< "GetFieldType(): Illegal field index " << column);
    return -1;
  }
  else
  {
    switch (sqlite3_column_type(this->Private->Statement, column))
    {
      case SQLITE_INTEGER:
        return VTK_INT;
      case SQLITE_FLOAT:
        return VTK_FLOAT;
      case SQLITE_TEXT:
        return VTK_STRING;
      case SQLITE_BLOB:
        return VTK_STRING; // until we have a BLOB type of our own
      case SQLITE_NULL:
        return VTK_VOID; // ??? what makes sense here?
      default:
      {
        vtkErrorMacro(<< "GetFieldType(): Unknown data type "
                      << sqlite3_column_type(this->Private->Statement, column) << " from SQLite.");
        return VTK_VOID;
      }
    }
  }
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::NextRow()
{
  if (!this->IsActive())
  {
    vtkErrorMacro(<< "NextRow(): Query is not active!");
    return false;
  }

  if (this->InitialFetch)
  {
    vtkDebugMacro(<< "NextRow(): Initial fetch being handled.");
    this->InitialFetch = false;
    if (this->InitialFetchResult == SQLITE_DONE)
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
    int result = sqlite3_step(this->Private->Statement);
    if (result == SQLITE_DONE)
    {
      return false;
    }
    else if (result == SQLITE_ROW)
    {
      return true;
    }
    else
    {
      vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);
      assert(dbContainer != nullptr);
      sqlite3* db = dbContainer->Internal->SQLiteInstance;
      this->SetLastErrorText(sqlite3_errmsg(db));
      vtkErrorMacro(<< "NextRow(): Database returned error code " << result
                    << " with the following message: " << this->GetLastErrorText());
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
    vtkWarningMacro(<< "DataValue() called on inactive query");
    return vtkVariant();
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    vtkWarningMacro(<< "DataValue() called with out-of-range column index " << column);
    return vtkVariant();
  }
  else
  {
    switch (sqlite3_column_type(this->Private->Statement, column))
    {
      case SQLITE_INTEGER:
        return vtkVariant(sqlite3_column_int(this->Private->Statement, column));

      case SQLITE_FLOAT:
        return vtkVariant(sqlite3_column_double(this->Private->Statement, column));

      case SQLITE_TEXT:
      {
        std::ostringstream str;
        str << sqlite3_column_text(this->Private->Statement, column);
        return vtkVariant(vtkStdString(str.str()));
      }

      case SQLITE_BLOB:
      {
        // This is a hack ... by passing the BLOB to vtkStdString with an explicit
        // byte count, we ensure that the string will store all of the BLOB's bytes,
        // even if there are nullptr values.

        return vtkVariant(vtkStdString(
          static_cast<const char*>(sqlite3_column_blob(this->Private->Statement, column)),
          sqlite3_column_bytes(this->Private->Statement, column)));
      }

      case SQLITE_NULL:
      default:
        return vtkVariant();
    }
  }
}

// ----------------------------------------------------------------------
const char* vtkSQLiteQuery::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::HasError()
{
  return (this->GetLastErrorText() != nullptr);
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::BeginTransaction()
{
  if (this->TransactionInProgress)
  {
    vtkErrorMacro(<< "Cannot start a transaction.  One is already in progress.");
    return false;
  }

  vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);

  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, BEGIN_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = true;
    this->SetLastErrorText(nullptr);
    vtkDebugMacro(<< "BeginTransaction() succeeded.");
    return true;
  }
  else
  {
    vtkErrorMacro(<< "BeginTransaction(): sqlite3_exec returned unexpected result code " << result);
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
  if (this->Private->Statement)
  {
    sqlite3_finalize(this->Private->Statement);
    this->Private->Statement = nullptr;
  }

  if (!this->TransactionInProgress)
  {
    vtkErrorMacro(<< "Cannot commit.  There is no transaction in progress.");
    return false;
  }

  vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);
  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, COMMIT_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = false;
    this->SetLastErrorText(nullptr);
    vtkDebugMacro(<< "CommitTransaction() succeeded.");
    return true;
  }
  else
  {
    vtkErrorMacro(<< "CommitTransaction(): sqlite3_exec returned unexpected result code "
                  << result);
    if (errorMessage)
    {
      this->SetLastErrorText(errorMessage);
      vtkErrorMacro(<< " and error message " << errorMessage);
    }
    assert(1 == 0);
    return false;
  }
}

// ----------------------------------------------------------------------
bool vtkSQLiteQuery::RollbackTransaction()
{
  if (!this->TransactionInProgress)
  {
    vtkErrorMacro(<< "Cannot rollback.  There is no transaction in progress.");
    return false;
  }

  vtkSQLiteDatabase* dbContainer = vtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);
  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, ROLLBACK_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = false;
    this->SetLastErrorText(nullptr);
    vtkDebugMacro(<< "RollbackTransaction() succeeded.");
    return true;
  }
  else
  {
    vtkErrorMacro(<< "RollbackTransaction(): sqlite3_exec returned unexpected result code "
                  << result);
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

bool vtkSQLiteQuery::BindParameter(int index, unsigned long long value)
{
  return this->BindInt64Parameter(index, static_cast<vtkTypeInt64>(value));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, long long value)
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

bool vtkSQLiteQuery::BindParameter(int index, const char* value)
{
  return this->BindParameter(index, value, strlen(value));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const char* data, size_t length)
{
  return this->BindStringParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const vtkStdString& value)
{
  return this->BindParameter(index, value.c_str());
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, const void* data, size_t length)
{
  return this->BindBlobParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindIntegerParameter(int index, int value)
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }
  int status = sqlite3_bind_int(this->Private->Statement, index + 1, value);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_int returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< errormessage.str().c_str());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindInt64Parameter(int index, vtkTypeInt64 value)
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }
  int status =
    sqlite3_bind_int(this->Private->Statement, index + 1, static_cast<sqlite_int64>(value));

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_int64 returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindDoubleParameter(int index, double value)
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status = sqlite3_bind_double(this->Private->Statement, index + 1, value);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_double returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindStringParameter(int index, const char* value, int length)
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status =
    sqlite3_bind_text(this->Private->Statement, index + 1, value, length, SQLITE_TRANSIENT);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_text returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindBlobParameter(int index, const void* data, int length)
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status =
    sqlite3_bind_blob(this->Private->Statement, index + 1, data, length, SQLITE_TRANSIENT);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_blob returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::ClearParameterBindings()
{
  if (!this->Private->Statement)
  {
    vtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status = sqlite3_clear_bindings(this->Private->Statement);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_clear_bindings returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool vtkSQLiteQuery::BindParameter(int index, vtkVariant value)
{
  return this->Superclass::BindParameter(index, value);
}
