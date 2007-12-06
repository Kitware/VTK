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

#include "vtkMySQLDatabase.h"
#include "vtkMySQLQuery.h"
#include <vtkStringArray.h>
#include <vtkObjectFactory.h>


#include <assert.h>
#include <mysql.h>
 
// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkMySQLDatabase, "1.1");
vtkStandardNewMacro(vtkMySQLDatabase);

// ----------------------------------------------------------------------

vtkMySQLDatabase::vtkMySQLDatabase()
  : HostName(NULL),
    UserName(NULL),
    Password(NULL),
    DatabaseName(NULL),
    Port(3306){
  this->Connection = NULL;
  mysql_init(& this->NullConnection);
  this->Tables = vtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();
  this->LastErrorText = NULL;
}

// ----------------------------------------------------------------------

vtkMySQLDatabase::~vtkMySQLDatabase()
{
  this->Close();
  this->SetHostName(NULL);
  this->SetUserName(NULL);
  this->SetPassword(NULL);
  this->SetLastErrorText(NULL);
  this->Tables->UnRegister(this);

}

// ----------------------------------------------------------------------

// The reason I'm writing this method out instead of just using
// vtkSetStringMacro is that I want to zero out the old string before
// I delete it.  That way there's less chance that a password will be
// left floating around in main memory.

void
vtkMySQLDatabase::SetPassword(const char *pwd)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << "Password to " << (pwd?pwd:"(null)") );      
  
  if ( this->Password == NULL && pwd == NULL) { return;}               
  if ( this->Password && pwd && (!strcmp(this->Password,pwd))) { return;} 

  if (this->Password) 
    { 
    memset(this->Password, 0, strlen(this->Password) * sizeof(char));
    delete [] this->Password;
    this->Password = NULL;
    }

  if (pwd) 
    { 
    size_t n = strlen(pwd) + 1;
    char *cp1 =  new char[n]; 
    const char *cp2 = (pwd); 
    this->Password = cp1; 
    do { *cp1++ = *cp2++; } while ( --n ); 
    } 
   else 
    { 
    this->Password = NULL; 
    } 
  this->Modified(); 
}

// ----------------------------------------------------------------------

bool
vtkMySQLDatabase::IsSupported(int feature)
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

bool
vtkMySQLDatabase::Open()
{
  bool canOpen = true;

  if (this->UserName == NULL)
    {
    vtkErrorMacro(<<"Open(): Username is null!");
    canOpen = false;
    }

  if (this->Password == NULL)
    {
    vtkErrorMacro(<<"Open(): Password is null!  For no password, use the string \"\".");
    canOpen = false;
    }

  if (this->HostName == NULL)
    {
    vtkErrorMacro(<<"Open(): Hostname is null!");
    canOpen = false;
    }
  
  if (this->DatabaseName == NULL)
    {
    vtkWarningMacro(<<"Open(): Database name is null.  This is permitted but highly discouraged.");
    }

  if (this->IsOpen())
    {
    vtkWarningMacro(<<"Open(): Database is already open.");
    return true;
    }

  if (!canOpen)
    {
    return false;
    }

  assert(this->Connection == NULL);

  this->Connection = 
    mysql_real_connect( & this->NullConnection, 
                        this->HostName,
                        this->UserName,
                        this->Password, 
                        (this->DatabaseName ? this->DatabaseName : ""),
                        this->Port,
                        0, 0);
                                        
  if (this->Connection == NULL)
    {
    this->SetLastErrorText(mysql_error(& this->NullConnection));
    vtkErrorMacro(<<"Open() failed with error " 
                  << this->LastErrorText);
    return false;
    }
  else
    {
    vtkDebugMacro(<<"Open() succeeded.");
    return true;
    }
}

// ----------------------------------------------------------------------

void
vtkMySQLDatabase::Close()
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

bool
vtkMySQLDatabase::IsOpen()
{
  return (this->Connection != NULL);
}

// ----------------------------------------------------------------------

vtkSQLQuery *
vtkMySQLDatabase::GetQueryInstance()
{
  vtkMySQLQuery *query = 
    vtkMySQLQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------

const char *
vtkMySQLDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------

vtkStringArray *
vtkMySQLDatabase::GetTables()
{
  this->Tables->Resize(0);
  if (!this->IsOpen())
    {
    vtkErrorMacro(<<"GetTables(): Database is closed!");
    return this->Tables;
    }
  else
    {
    MYSQL_RES *tableResult = mysql_list_tables(this->Connection,
                                               NULL);

    if (!tableResult)
      {
      this->SetLastErrorText(mysql_error(this->Connection));
      vtkErrorMacro(<<"GetTables(): MySQL returned error: "
                    << this->LastErrorText);
      return this->Tables;
      }

    MYSQL_ROW row;
    int i=0;

    while (tableResult)
      {
      mysql_data_seek(tableResult, i);
      row = mysql_fetch_row(tableResult);
      if (!row) 
        {
        break;
        }

      this->Tables->InsertNextValue(row[0]);
      ++i;

      mysql_free_result(tableResult);
      }

    return this->Tables;
    }
}

// ----------------------------------------------------------------------

vtkStringArray *
vtkMySQLDatabase::GetRecord(const char *table)
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
    this->SetLastErrorText(mysql_error(this->Connection));
    vtkErrorMacro(<<"GetRecord: MySQL returned error: "
                  << this->LastErrorText);
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

// ----------------------------------------------------------------------

void
vtkMySQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
