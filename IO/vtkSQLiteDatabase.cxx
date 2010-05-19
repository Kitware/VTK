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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkSQLiteDatabase.h"
#include "vtkSQLiteQuery.h"

#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>

#include <vtksqlite/vtk_sqlite3.h>

vtkStandardNewMacro(vtkSQLiteDatabase);

// ----------------------------------------------------------------------
vtkSQLiteDatabase::vtkSQLiteDatabase()
{
  this->SQLiteInstance = NULL;

  this->Tables = vtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();

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
  this->Tables->UnRegister(this);
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
  vtksys_ios::ostringstream queryStr;
  queryStr << schema->GetColumnNameFromHandle( tblHandle, colHandle );

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle ); 
  vtkStdString colTypeStr;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    
      colTypeStr = "INTEGER NOT NULL";
      break;
    case vtkSQLDatabaseSchema::SMALLINT:  
      colTypeStr = "SMALLINT";
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colTypeStr = "INTEGER";
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colTypeStr = "BIGINT";
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colTypeStr = "VARCHAR";
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colTypeStr = "TEXT";
      break;
    case vtkSQLDatabaseSchema::REAL:      
      colTypeStr = "REAL";
      break;
    case vtkSQLDatabaseSchema::DOUBLE:    
      colTypeStr = "DOUBLE";
      break;
    case vtkSQLDatabaseSchema::BLOB:      
      colTypeStr = "BLOB";
      break;
    case vtkSQLDatabaseSchema::TIME:      
      colTypeStr = "TIME";
      break;
    case vtkSQLDatabaseSchema::DATE:      
      colTypeStr = "DATE";
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP: 
      colTypeStr = "TIMESTAMP";
    }
  
  if ( colTypeStr.size() )
    {
    queryStr << " " << colTypeStr;
    }
  else // if ( colTypeStr.size() )
    {
    vtkGenericWarningMacro( "Unable to get column specification: unsupported data type " << colType );
    return vtkStdString();
    }
  
  // Decide whether size is allowed, required, or unused
  int colSizeType = 0;
  switch ( static_cast<vtkSQLDatabaseSchema::DatabaseColumnType>( colType ) )
    {
    case vtkSQLDatabaseSchema::SERIAL:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::SMALLINT:  
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colSizeType = -1;
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::REAL:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::DOUBLE:    
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::BLOB:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::TIME:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::DATE:      
      colSizeType =  0;
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP: 
      colSizeType =  0;
      break;
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
    if ( colSize > 0 )
      {
      queryStr << "(" << colSize << ")";
      }
    }

  vtkStdString attStr = schema->GetColumnAttributesFromHandle( tblHandle, colHandle );
  if ( attStr.size() )
    {
    queryStr << " " << attStr;
    }

  return queryStr.str();
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
    case VTK_SQL_FEATURE_TRIGGERS:
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
bool vtkSQLiteDatabase::Open(const char* password)
{
  return this->Open(password, USE_EXISTING);
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::Open(const char* password, int mode)
{
  if (this->IsOpen())
    {
    vtkWarningMacro("Open(): Database is already open.");
    return true;
    }

  if(password && strlen(password))
    {
    vtkGenericWarningMacro("Password will be ignored by vtkSQLiteDatabase::Open().");
    }

  if (!this->DatabaseFileName)
    {
    vtkErrorMacro("Cannot open database because DatabaseFileName is not set.");
    return false;
    }

  if (this->IsOpen())
    {
    vtkGenericWarningMacro( "Open(): Database is already open." );
    return true;
    }

  // Only do checks if it is not an in-memory database
  if (strcmp(":memory:", this->DatabaseFileName))
    {
    bool exists = vtksys::SystemTools::FileExists(this->DatabaseFileName);
    if (mode == USE_EXISTING && !exists)
      {
      vtkErrorMacro("You specified using an existing database but the file does not exist.\n"
                    "Use USE_EXISTING_OR_CREATE to allow database creation.");
      return false;
      }
    if (mode == CREATE && exists)
      {
      vtkErrorMacro("You specified creating a database but the file exists.\n"
                    "Use USE_EXISTING_OR_CREATE to allow using an existing database,\n"
                    "or CREATE_OR_CLEAR to clear any existing file.");
      return false;
      }
    if (mode == CREATE_OR_CLEAR && exists)
      {
      // Here we need to clear the file if it exists by opening it.
      vtksys_ios::ofstream os;
      os.open(this->DatabaseFileName);
      if (!os.is_open())
        {
        vtkErrorMacro("Unable to create file " << this->DatabaseFileName << ".");
        return false;
        }
      os.close();
      }
    }

  int result = vtk_sqlite3_open(this->DatabaseFileName, & (this->SQLiteInstance));

  if (result != VTK_SQLITE_OK)
    {
    vtkDebugMacro(<<"SQLite open() failed.  Error code is " 
                  << result << " and message is " 
                  << vtk_sqlite3_errmsg(this->SQLiteInstance) );

    vtk_sqlite3_close(this->SQLiteInstance);
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
  this->Tables->Resize(0);
  if (this->SQLiteInstance == NULL)
    {
    vtkErrorMacro(<<"GetTables(): Database is not open!");
    return this->Tables;
    }

  vtkSQLQuery *query = this->GetQueryInstance();
  query->SetQuery("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
  bool status = query->Execute();

  if (!status)
    {
    vtkErrorMacro(<< "GetTables(): Database returned error: "
                  << vtk_sqlite3_errmsg(this->SQLiteInstance) );
    query->Delete();
    return this->Tables;
    }
  else
    {
    vtkDebugMacro(<<"GetTables(): SQL query succeeded.");
    while (query->NextRow() )
      {
      this->Tables->InsertNextValue(query->DataValue(0).ToString() );
      }
    query->Delete();
    return this->Tables;
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
  const char* fname = this->GetDatabaseFileName();
  this->TempURL = this->GetDatabaseType();
  this->TempURL += "://";
  if ( fname )
    {
    this->TempURL += fname;
    }
  return this->TempURL;
}

// ----------------------------------------------------------------------
bool vtkSQLiteDatabase::ParseURL(const char* URL)
{
  vtkstd::string protocol;
  vtkstd::string dataglom;
  
  if ( ! vtksys::SystemTools::ParseURLProtocol( URL, protocol, dataglom))
    {
    vtkErrorMacro( "Invalid URL: " << URL );
    return false;
    }

  if ( protocol == "sqlite" )
    {
    this->SetDatabaseFileName(dataglom.c_str());
    return true;
    }

  return false;
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
