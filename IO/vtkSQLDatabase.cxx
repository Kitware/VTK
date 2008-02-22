/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabase.cxx

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

#include "vtkToolkits.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"

#include "vtkSQLDatabaseSchema.h"

#include "vtkSQLiteDatabase.h"

#ifdef VTK_USE_POSTGRES
#include "vtkPostgreSQLDatabase.h"
#endif // VTK_USE_POSTGRES

#ifdef VTK_USE_MYSQL
#include "vtkMySQLDatabase.h"
#endif // VTK_USE_MYSQL

#include "vtkObjectFactory.h"
#include "vtkStdString.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkSQLDatabase, "1.34");

// ----------------------------------------------------------------------
vtkSQLDatabase::vtkSQLDatabase()
{
}

// ----------------------------------------------------------------------
vtkSQLDatabase::~vtkSQLDatabase()
{
}

// ----------------------------------------------------------------------
void vtkSQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
vtkStdString vtkSQLDatabase::GetColumnSpecification( vtkSQLDatabaseSchema* schema,
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
      colTypeStr = "";
      break;
    case vtkSQLDatabaseSchema::SMALLINT:  
      colTypeStr = "INTEGER";
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colTypeStr = "INTEGER";
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colTypeStr = "INTEGER";
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colTypeStr = "VARCHAR";
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colTypeStr = "VARCHAR";
      break;
    case vtkSQLDatabaseSchema::REAL:      
      colTypeStr = "FLOAT";
      break;
    case vtkSQLDatabaseSchema::DOUBLE:    
      colTypeStr = "DOUBLE";
      break;
    case vtkSQLDatabaseSchema::BLOB:      
      colTypeStr = "";
      break;
    case vtkSQLDatabaseSchema::TIME:      
      colTypeStr = "TIME";
      break;
    case vtkSQLDatabaseSchema::DATE:      
      colTypeStr = "DATE";
      break;
    case vtkSQLDatabaseSchema::TIMESTAMP: 
      colTypeStr = "TIMESTAMP";
      break;
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
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::INTEGER:   
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::BIGINT:    
      colSizeType =  1;
      break;
    case vtkSQLDatabaseSchema::VARCHAR:   
      colSizeType = -1;
      break;
    case vtkSQLDatabaseSchema::TEXT:      
      colSizeType = -1;
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
vtkStdString vtkSQLDatabase::GetIndexSpecification( vtkSQLDatabaseSchema* schema,
                                                    int tblHandle,
                                                    int idxHandle,
                                                    bool& skipped )
{
  vtkStdString queryStr;

  int idxType = schema->GetIndexTypeFromHandle( tblHandle, idxHandle );
  switch ( idxType )
    {
    case vtkSQLDatabaseSchema::PRIMARY_KEY:
      queryStr = ", PRIMARY KEY ";
      skipped = false;
      break;
    case vtkSQLDatabaseSchema::UNIQUE:
      queryStr = ", UNIQUE ";
      skipped = false;
      break;
    case vtkSQLDatabaseSchema::INDEX:
      // Not supported within a CREATE TABLE statement by all SQL backends: 
      // must be created later with a CREATE INDEX statement
      queryStr = "CREATE INDEX ";
      skipped = true;
      break;
    default:
      return vtkStdString();
    }
  
  queryStr += schema->GetIndexNameFromHandle( tblHandle, idxHandle );

  // CREATE INDEX <index name> ON <table name> syntax
  if ( skipped )
    {
    queryStr += " ON ";
    queryStr += schema->GetTableNameFromHandle( tblHandle );
    }

  queryStr += " (";
        
  // Loop over all column names of the index
  int numCnm = schema->GetNumberOfColumnNamesInIndex( tblHandle, idxHandle );
  if ( numCnm < 0 )
    {
    vtkGenericWarningMacro( "Unable to get index specification: index has incorrect number of columns " << numCnm );
    return vtkStdString();
    }

  bool firstCnm = true;
  for ( int cnmHandle = 0; cnmHandle < numCnm; ++ cnmHandle )
    {
    if ( firstCnm )
      {
      firstCnm = false;
      }
    else
      {
      queryStr += ",";
      }
    queryStr += schema->GetIndexColumnNameFromHandle( tblHandle, idxHandle, cnmHandle );
    }
  queryStr += ")";

  return queryStr;
}

// ----------------------------------------------------------------------
vtkSQLDatabase* vtkSQLDatabase::CreateFromURL( const char* URL )
{
  vtkstd::string protocol;
  vtkstd::string username; 
  vtkstd::string password;
  vtkstd::string hostname; 
  vtkstd::string dataport; 
  vtkstd::string database;
  vtkstd::string dataglom;
  vtkSQLDatabase* db = 0;
  
  // SQLite is a bit special so lets get that out of the way :)
  if ( ! vtksys::SystemTools::ParseURLProtocol( URL, protocol, dataglom))
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return 0;
    }
  if ( protocol == "sqlite" )
    {
    db = vtkSQLiteDatabase::New();
    vtkSQLiteDatabase *sqlite_db = vtkSQLiteDatabase::SafeDownCast(db);
    sqlite_db->SetDatabaseFileName(dataglom.c_str());
    return db;
    }
    
  // Okay now for all the other database types get more detailed info
  if ( ! vtksys::SystemTools::ParseURL( URL, protocol, username,
                                        password, hostname, dataport, database) )
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return 0;
    }
  
#ifdef VTK_USE_POSTGRES
  if ( protocol == "psql" )
    {
    db = vtkPostgreSQLDatabase::New();
    vtkPostgreSQLDatabase *post_db = vtkPostgreSQLDatabase::SafeDownCast(db);
    post_db->SetUser(username.c_str());
    post_db->SetPassword(password.c_str());
    post_db->SetHostName(hostname.c_str());
    post_db->SetServerPort(atoi(dataport.c_str()));
    post_db->SetDatabaseName(database.c_str());
    return db;
    }
#endif // VTK_USE_POSTGRES
#ifdef VTK_USE_MYSQL
  if ( protocol == "mysql" )
    {
    db = vtkMySQLDatabase::New();
    vtkMySQLDatabase *mysql_db = vtkMySQLDatabase::SafeDownCast(db);
    if ( username.size() )
      {
      mysql_db->SetUser(username.c_str());
      }
    if ( password.size() )
      {
      mysql_db->SetPassword(password.c_str());
      }
    if ( dataport.size() )
      {
      mysql_db->SetServerPort(atoi(dataport.c_str()));
      }
    mysql_db->SetHostName(hostname.c_str());
    mysql_db->SetDatabaseName(database.c_str());
    return db;
    }
#endif // VTK_USE_MYSQL

  vtkGenericWarningMacro( "Unsupported protocol: " << protocol.c_str() );
  return db;
}

// ----------------------------------------------------------------------
bool vtkSQLDatabase::EffectSchema( vtkSQLDatabaseSchema* schema, bool dropIfExists )
{
  if ( ! this->IsOpen() )
    {
    vtkGenericWarningMacro( "Unable to effect the schema: no database is open" );
    return false;
    }

  // Instantiate an empty query and begin the transaction.
  vtkSQLQuery* query = this->GetQueryInstance();
  if ( ! query->BeginTransaction() )
    {
    vtkGenericWarningMacro( "Unable to effect the schema: unable to begin transaction" );
    return false;
    }
 
  // In case INDEX indices are encountered in the schema
  vtkstd::vector<vtkStdString> idxStatements;

  // Loop over all tables of the schema and create them
  int numTbl = schema->GetNumberOfTables();
  for ( int tblHandle = 0; tblHandle < numTbl; ++ tblHandle )
    {
    // Construct the query string for this table
    vtkStdString queryStr( "CREATE TABLE " );
    queryStr += this->GetTablePreamble( dropIfExists );
    queryStr += schema->GetTableNameFromHandle( tblHandle );
    queryStr += " (";

    // Loop over all columns of the current table
    int numCol = schema->GetNumberOfColumnsInTable( tblHandle );
    if ( numCol < 0 )
      {
      query->RollbackTransaction();
      query->Delete();
      return false;
      }

    bool firstCol = true;
    for ( int colHandle = 0; colHandle < numCol; ++ colHandle )
      {
      if ( ! firstCol )
        {
        queryStr += ", ";
        }
      else // ( ! firstCol )
        {
        firstCol = false;
        }

      // Get column creation syntax (backend-dependent)
      vtkStdString colStr = this->GetColumnSpecification( schema, tblHandle, colHandle );
      if ( colStr.size() )
        {
        queryStr += colStr;
        }
      else // if ( colStr.size() )
        {
        query->RollbackTransaction();
        query->Delete();
        return false;
        }
      }

    // Loop over all indices of the current table
    bool skipped = false;
    int numIdx = schema->GetNumberOfIndicesInTable( tblHandle );
    if ( numIdx < 0 )
      {
      query->RollbackTransaction();
      query->Delete();
      return false;
      }
    for ( int idxHandle = 0; idxHandle < numIdx; ++ idxHandle )
      {
      // Get index creation syntax (backend-dependent)
      vtkStdString idxStr = this->GetIndexSpecification( schema, tblHandle, idxHandle, skipped );
      if ( idxStr.size() )
        {
        if ( skipped )
          {
          // Must create this index later
          idxStatements.push_back( idxStr );
          continue;
          }
        else // if ( skipped )
          {
          queryStr += idxStr;
          }
        }
      else // if ( idxStr.size() )
        {
        query->RollbackTransaction();
        query->Delete();
        return false;
        }
      }
    queryStr += ")";

    // Execute the query
    query->SetQuery( queryStr );
    if ( ! query->Execute() )
      {
      vtkGenericWarningMacro( "Unable to effect the schema: unable to execute query.\nDetails: "
                              << query->GetLastErrorText() );
      query->RollbackTransaction();
      query->Delete();
      return false;
      }
    }

  // Now, execute the CREATE INDEX statement -- if any
  for ( vtkstd::vector<vtkStdString>::iterator it = idxStatements.begin();
        it != idxStatements.end(); ++ it )
    {
    query->SetQuery( *it );
    if ( ! query->Execute() )
      {
      vtkGenericWarningMacro( "Unable to effect the schema: unable to execute query.\nDetails: "
                              << query->GetLastErrorText() );
      query->RollbackTransaction();
      query->Delete();
      return false;
      }
    }
 
  // FIXME: eventually handle triggers

  // Commit the transaction.
  if ( ! query->CommitTransaction() )
    {
    vtkGenericWarningMacro( "Unable to effect the schema: unable to commit transaction.\nDetails: "
                            << query->GetLastErrorText() );
    query->Delete();
    return false;
    }

  query->Delete();
  return true;
}

