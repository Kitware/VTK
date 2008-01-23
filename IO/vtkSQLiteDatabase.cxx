/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteDatabase.cxx

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
#include "vtkSQLiteDatabase.h"
#include "vtkSQLiteQuery.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#include <vtksqlite/vtk_sqlite3.h>

vtkStandardNewMacro(vtkSQLiteDatabase);
vtkCxxRevisionMacro(vtkSQLiteDatabase, "1.5");

// ----------------------------------------------------------------------
vtkSQLiteDatabase::vtkSQLiteDatabase()
{
  this->SQLiteInstance = NULL;

  // Initialize instance variables
  this->DatabaseType = 0;
  this->SetDatabaseType("sqlite");
  this->DatabaseFileName = 0;
}

// ----------------------------------------------------------------------
vtkSQLiteDatabase::~vtkSQLiteDatabase()
{
  if (this->IsOpen() )
    {
    this->Close();
    }
  if ( this->DatabaseType )
    {
    this->SetDatabaseType(0);
    }
  if ( this->DatabaseFileName )
    {
    this->SetDatabaseFileName(0);
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::IsSupported(int feature)
{
  switch (feature)
    {
    case VTK_SQL_FEATURE_BLOB:
    case VTK_SQL_FEATURE_LAST_INSERT_ID:
    case VTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
    case VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
    case VTK_SQL_FEATURE_PREPARED_QUERIES:
    case VTK_SQL_FEATURE_TRANSACTIONS:
    case VTK_SQL_FEATURE_UNICODE:
      return true;
      
    case VTK_SQL_FEATURE_BATCH_OPERATIONS:
    case VTK_SQL_FEATURE_QUERY_SIZE:
      return false;

    default:
      {
      vtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                    << "vtkSQLDatabase.h for a list of possible features.");
      return false;
      };
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::Open()
{

  if ( this->IsOpen() )
    {
    vtkGenericWarningMacro( "Open(): Database is already open." );
    return true;
    }

  if (!this->DatabaseFileName)
    {
    vtkErrorMacro("Cannot open database because DatabaseFileName is not set.");
    return false;
    }

  int result = vtk_sqlite3_open( this->DatabaseFileName, & (this->SQLiteInstance) );

  if ( result != VTK_SQLITE_OK )
    {
    vtkDebugMacro(<<"SQLite open() failed.  Error code is " 
                  << result << " and message is " 
                  << vtk_sqlite3_errmsg(this->SQLiteInstance) );

    vtk_sqlite3_close( this->SQLiteInstance );
    return false;
    }
  else
    {
    vtkDebugMacro(<<"SQLite open() succeeded.");
    return true;
    }
}

// ----------------------------------------------------------------------
void vtkSQLiteDatabase::Close()
{
  if (this->SQLiteInstance == NULL)
    {
    vtkDebugMacro(<<"Close(): Database is already closed.");
    }
  else
    {
    int result = vtk_sqlite3_close(this->SQLiteInstance);
    if (result != VTK_SQLITE_OK)
      {
      vtkWarningMacro(<< "Close(): SQLite returned result code " << result);
      }
    this->SQLiteInstance = NULL;
    }
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::IsOpen()
{
  return (this->SQLiteInstance != NULL);
}

// ----------------------------------------------------------------------
vtkSQLQuery * vtkSQLiteDatabase::GetQueryInstance()
{
  vtkSQLiteQuery *query = vtkSQLiteQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------
vtkStringArray * vtkSQLiteDatabase::GetTables()
{
  if (this->SQLiteInstance == NULL)
    {
    vtkErrorMacro(<<"GetTables(): Database is not open!");
    return NULL;
    }

  vtkSQLQuery *query = this->GetQueryInstance();
  query->SetQuery("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
  bool status = query->Execute();

  if (!status)
    {
    vtkErrorMacro(<< "GetTables(): Database returned error: "
                  << vtk_sqlite3_errmsg(this->SQLiteInstance) );
    query->Delete();
    return NULL;
    }
  else
    {
    vtkDebugMacro(<<"GetTables(): SQL query succeeded.");
    vtkStringArray *results = vtkStringArray::New();
    while (query->NextRow() )
      {
      results->InsertNextValue(query->DataValue(0).ToString() );
      }
    query->Delete();
    return results;
    }
}

// ----------------------------------------------------------------------
vtkStringArray * vtkSQLiteDatabase::GetRecord(const char *table)
{
  vtkSQLQuery *query = this->GetQueryInstance();
  vtkStdString text("PRAGMA table_info ('");
  text += table;
  text += "')";

  query->SetQuery(text.c_str() );
  bool status = query->Execute();
  if (!status)
    {
    vtkErrorMacro(<< "GetRecord(" << table << "): Database returned error: "
                  << vtk_sqlite3_errmsg(this->SQLiteInstance) );
    query->Delete();
    return NULL;
    }
  else
    {
    // Each row in the results that come back from this query
    // describes a single column in the table.  The format of each row
    // is as follows:
    //
    // columnID columnName columnType ??? defaultValue nullForbidden
    // 
    // (I don't know what the ??? column is.  It's probably maximum
    // length.)
    vtkStringArray *results = vtkStringArray::New();
    
    while (query->NextRow() )
      {
      results->InsertNextValue(query->DataValue(1).ToString() );
      }

    query->Delete();
    return results;
    }
}

// ----------------------------------------------------------------------
vtkStdString vtkSQLiteDatabase::GetURL()
{
  vtkStdString url;
  url = this->GetDatabaseType();
  url += "://";
  url += this->GetDatabaseFileName();
  return url;
}

bool vtkSQLiteDatabase::HasError()
{ 
  return (vtk_sqlite3_errcode(this->SQLiteInstance)!=VTK_SQLITE_OK);
}

const char* vtkSQLiteDatabase::GetLastErrorText()
{
  return vtk_sqlite3_errmsg(this->SQLiteInstance);
}

// ----------------------------------------------------------------------
void vtkSQLiteDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SQLiteInstance: ";
  if (this->SQLiteInstance)
    {
    cout << this->SQLiteInstance << "\n";
    }
  else
    {
    cout << "(null)" << "\n";
    }
  os << indent << "DatabaseType: " 
    << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
  os << indent << "DatabaseFileName: " 
    << (this->DatabaseFileName ? this->DatabaseFileName : "NULL") << endl;
}



  
    
