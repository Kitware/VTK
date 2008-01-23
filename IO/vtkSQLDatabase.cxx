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

#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkSQLDatabase, "1.9");

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
    post_db->SetPort(atoi(dataport.c_str()));
    post_db->SetDatabaseName(database.c_str());
    }
#endif // VTK_USE_POSTGRES
#ifdef VTK_USE_MYSQL
  else if ( protocol == "mysql" )
    {
    db = vtkMySQLDatabase::New();
    vtkMySQLDatabase *mysql_db = vtkMySQLDatabase::SafeDownCast(db);
    mysql_db->SetUserName(username.c_str());
    mysql_db->SetPassword(password.c_str());
    mysql_db->SetHostName(hostname.c_str());
    mysql_db->SetPort(atoi(dataport.c_str()));
    mysql_db->SetDatabaseName(database.c_str());
    }
#endif // VTK_USE_MYSQL
  else
    {
    vtkGenericWarningMacro( "Unsupported protocol: " << protocol.c_str() );
    return 0;
    }

  return db;
}

