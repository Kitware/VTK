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

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#include <assert.h>
#include <mysql.h>
 
vtkCxxRevisionMacro(vtkMySQLDatabase, "1.3");
vtkStandardNewMacro(vtkMySQLDatabase);

// ----------------------------------------------------------------------
vtkMySQLDatabase::vtkMySQLDatabase()
{
  this->URL = NULL;
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
  if ( this->IsOpen() )
    {
    this->Close();
    }

  this->SetURL( NULL );
  this->SetLastErrorText(NULL);
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
  if  ( ! this->URL )
    {
    this->SetLastErrorText("Cannot open database because URL is null.");
    vtkErrorMacro(<< this->GetLastErrorText());
    return false;
    }

  if ( this->IsOpen() )
    {
    vtkGenericWarningMacro( "Open(): Database is already open." );
    return true;
    }

  assert(this->Connection == NULL);

  vtkstd::string protocol;
  vtkstd::string username;
  vtkstd::string password;
  vtkstd::string hostname;
  vtkstd::string dataport;
  vtkstd::string database;

  bool parsing = vtksys::SystemTools::ParseURL( static_cast<vtkstd::string>( this->URL ),
                                               protocol, username, password,
                                               hostname, dataport, database );
  if ( ! parsing || protocol != "mysql" )
    {
    vtkGenericWarningMacro( "Invalid URL: " << this->URL );
    return 0;
    }

  this->Connection = 
    mysql_real_connect( & this->NullConnection, 
                        hostname.c_str(),
                        username.c_str(),
                        password.c_str(), 
                        database.c_str(),
                        atoi( dataport.c_str() ),
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
  vtkMySQLQuery *query = 
    vtkMySQLQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------
const char* vtkMySQLDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------
vtkStringArray* vtkMySQLDatabase::GetTables()
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
void vtkMySQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
