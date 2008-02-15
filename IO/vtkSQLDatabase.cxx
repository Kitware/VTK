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
#include "vtkSQLiteDatabase.h"

#ifdef VTK_USE_POSTGRES
#include "vtkPostgreSQLDatabase.h"
#endif // VTK_USE_POSTGRES

#ifdef VTK_USE_MYSQL
#include "vtkMySQLDatabase.h"
#endif // VTK_USE_MYSQL

#include "vtkSQLDatabaseSchema.h"
#include "vtkSQLQuery.h"

#include "vtkObjectFactory.h"
#include "vtkStdString.h"

#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkSQLDatabase, "1.17");

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
  
  // Sqlite is a bit special so lets get that out of the way :)
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
    post_db->SetUserName(username.c_str());
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
    mysql_db->SetUserName(username.c_str());
    mysql_db->SetPassword(password.c_str());
    mysql_db->SetHostName(hostname.c_str());
    mysql_db->SetServerPort(atoi(dataport.c_str()));
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
  dropIfExists = false; // Unused for now

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
 
  // Loop over all tables of the schema and create them
  int numTbl = schema->GetNumberOfTables();
  for ( int tblHandle = 0; tblHandle < numTbl; ++ tblHandle )
    {
    // Construct the query string for this table
    vtkStdString queryStr( "CREATE TABLE " );
    queryStr += schema->GetTableNameFromHandle( tblHandle );
    queryStr += " (";

    // Loop over all columns of the current table
    bool firstCol = true;
    int numCol = schema->GetNumberOfColumnsInTable( tblHandle );
    if ( numCol < 0 )
      {
      query->RollbackTransaction();
      return false;
      }
    for ( int colHandle = 0; colHandle < numCol; ++ colHandle )
      {
      if ( ! firstCol )
        {
        queryStr += ", ";
        }
      else
        {
        firstCol = false;
        }
      
      queryStr += schema->GetColumnNameFromHandle( tblHandle, colHandle );
      queryStr += " ";

      int colType = schema->GetColumnTypeFromHandle( tblHandle, colHandle ); 

      vtkStdString colStr = this->GetColumnTypeString( colType );
      if ( colStr )
        {
        queryStr += " ";
        queryStr += colStr;
        }
      else // if ( colStr )
        {
        vtkGenericWarningMacro( "Unable to effect the schema: unsupported data type " << colType );
        query->RollbackTransaction();
        return false;
        }

      vtkStdString attStr = schema->GetColumnAttributesFromHandle( tblHandle, colHandle );
      if ( attStr )
        {
        queryStr += " ";
        queryStr += attStr;
        }
      }

    // Loop over all indices of the current table
    int numIdx = schema->GetNumberOfIndicesInTable( tblHandle );
    if ( numIdx < 0 )
      {
      query->RollbackTransaction();
      return false;
      }
    for ( int idxHandle = 0; idxHandle < numIdx; ++ idxHandle )
      {
      queryStr += ", ";
      switch ( schema->GetIndexTypeFromHandle( tblHandle, idxHandle ) )
        {
        case 0:
          queryStr += "PRIMARY KEY ";
          break;
        case 1:
          queryStr += "UNIQUE ";
          break;
        case 2:
          queryStr += "INDEX ";
          break;
        default:
          query->RollbackTransaction();
          return false;
        }

      queryStr += schema->GetIndexNameFromHandle( tblHandle, idxHandle );
      queryStr += " (";

      // Loop over all column names of the current index
      bool firstCnm = true;
      int numCnms = schema->GetNumberOfColumnNamesInIndex( tblHandle, idxHandle );
      if ( numCnms < 0 )
        {
        query->RollbackTransaction();
        return false;
        }
      for ( int cnmHandle = 0; cnmHandle < numCnms; ++ cnmHandle )
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
      }
    queryStr += ")";
    
    // Execute the query
    query->SetQuery( queryStr );
    if ( ! query->Execute() )
      {
      vtkGenericWarningMacro( "Unable to effect the schema: unable to execute query" );
      query->RollbackTransaction();
      return false;
      }
    }
  
  // FIXME: eventually handle triggers

  // Commit the transaction.
  if ( ! query->CommitTransaction() )
    {
    vtkGenericWarningMacro( "Unable to effect the schema: unable to commit transaction" );
    return false;
    }

  return true;
}
