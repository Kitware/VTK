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

#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#include <vtksqlite/vtk_sqlite3.h>

vtkStandardNewMacro(vtkSQLiteDatabase);
vtkCxxRevisionMacro(vtkSQLiteDatabase, "1.10");

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
void vtkSQLiteDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SQLiteInstance: ";
  if (this->SQLiteInstance)
    {
    os << this->SQLiteInstance << "\n";
    }
  else
    {
    os << "(null)" << "\n";
    }
  os << indent << "DatabaseType: " 
    << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
  os << indent << "DatabaseFileName: " 
    << (this->DatabaseFileName ? this->DatabaseFileName : "NULL") << endl;
}

// ----------------------------------------------------------------------
vtkStdString vtkSQLiteDatabase::GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                                     int tblHandle,
                                                     int colHandle )
{
  vtkStdString queryStr = schema->GetColumnNameFromHandle( tblHandle, colHandle );

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle ); 
  vtkStdString colTypeStr = 0;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    colTypeStr = 0;
    case vtkSQLDatabaseSchema::SMALLINT:  colTypeStr = "SMALLINT";
    case vtkSQLDatabaseSchema::INTEGER:   colTypeStr = "INTEGER";
    case vtkSQLDatabaseSchema::BIGINT:    colTypeStr = "BIGINT";
    case vtkSQLDatabaseSchema::VARCHAR:   colTypeStr = "VARCHAR";
    case vtkSQLDatabaseSchema::TEXT:      colTypeStr = "TEXT";
    case vtkSQLDatabaseSchema::REAL:      colTypeStr = "REAL";
    case vtkSQLDatabaseSchema::DOUBLE:    colTypeStr = "DOUBLE";
    case vtkSQLDatabaseSchema::BLOB:      colTypeStr = "BLOB";
    case vtkSQLDatabaseSchema::TIME:      colTypeStr = "TIME";
    case vtkSQLDatabaseSchema::DATE:      colTypeStr = "DATE";
    case vtkSQLDatabaseSchema::TIMESTAMP: colTypeStr = "TIMESTAMP";
    }
  
  if ( colTypeStr )
    {
    queryStr += " ";
    queryStr += colTypeStr;
    }
  else // if ( colTypeStr )
    {
    vtkGenericWarningMacro( "Unable to get column specification: unsupported data type " << colType );
    return 0;
    }
  
  // Decide whether size is allowed, required, or unused
  int colSizeType = 0;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    colSizeType =  0;
    case vtkSQLDatabaseSchema::SMALLINT:  colSizeType =  1;
    case vtkSQLDatabaseSchema::INTEGER:   colSizeType =  1;
    case vtkSQLDatabaseSchema::BIGINT:    colSizeType =  1;
    case vtkSQLDatabaseSchema::VARCHAR:   colSizeType = -1;
    case vtkSQLDatabaseSchema::TEXT:      colSizeType = -1;
    case vtkSQLDatabaseSchema::REAL:      colSizeType =  1;
    case vtkSQLDatabaseSchema::DOUBLE:    colSizeType =  1;
    case vtkSQLDatabaseSchema::BLOB:      colSizeType =  0;
    case vtkSQLDatabaseSchema::TIME:      colSizeType =  0;
    case vtkSQLDatabaseSchema::DATE:      colSizeType =  0;
    case vtkSQLDatabaseSchema::TIMESTAMP: colSizeType =  0;
    }

  // Specify size if allowed or required
  if ( colSizeType )
    {
    int colSize = schema->GetColumnSizeFromHandle( tblHandle, colHandle );
    // IF size is provided but absurd, 
    // OR, if size is required but not provided OR absurd,
    // THEN assign the default size.
    if ( ( colSize < 0 ) || ( colSizeType == -1 && colSize < 1 ) )
      {
      colSize = VTK_SQL_DEFAULT_COLUMN_SIZE;
      }
    
    // At this point, we have either a valid size if required, or a possibly null valid size
    // if not required. Thus, skip sizing in the latter case.
    if ( colSize < 0 )
      {
      queryStr += "(";
      queryStr += colSize;
      queryStr += ")";
      }
    }

  vtkStdString attStr = schema->GetColumnAttributesFromHandle( tblHandle, colHandle );
  if ( attStr )
    {
    queryStr += " ";
    queryStr += attStr;
    }

  return queryStr;
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
  const char* fname = this->GetDatabaseFileName();
  url = this->GetDatabaseType();
  url += "://";
  if ( fname )
    {
    url += fname;
    }
  return url;
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::HasError()
{ 
  return (vtk_sqlite3_errcode(this->SQLiteInstance)!=VTK_SQLITE_OK);
}

const char* vtkSQLiteDatabase::GetLastErrorText()
{
  return vtk_sqlite3_errmsg(this->SQLiteInstance);
}
