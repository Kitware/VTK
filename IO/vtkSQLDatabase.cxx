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

#include "vtkSQLDatabase.h"
#include "vtkSQLiteDatabase.h"

#ifdef VTK_USE_PSQL
#include "vtkPostgreSQLDatabase.h"
#endif // VTK_USE_PSQL

#ifdef VTK_USE_MYSQL
#include "vtkMySQLDatabase.h"
#endif // VTK_USE_MYSQL

#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>

vtkCxxRevisionMacro(vtkSQLDatabase, "1.3");

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
  vtkstd::string protocol( 0 );
  vtkstd::string dataglom( 0 );

  if ( ! vtksys::SystemTools::ParseURLProtocol( URL, protocol, dataglom ) )
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return 0;
    }
  
  vtkSQLDatabase* db = 0;
  if ( protocol == "sqlite" )
    {
    vtkSQLiteDatabase* sdb = vtkSQLiteDatabase::New();
    if ( sdb )
      {
      sdb->SetFileName( dataglom.c_str() );
      }

    db = sdb;
    }
  else if ( protocol == "psql" )
    {
    db = vtkPostgreSQLDatabase::New();
    }
  else if ( protocol == "mysql" )
    {
    db = vtkMySQLDatabase::New();
    }
  else
    {
    vtkGenericWarningMacro( "Unsupported protocol: " << protocol.c_str() );
    return 0;
    }

  if ( db )
    {
    db->SetURL( URL );
    }
  else
    {
    vtkGenericWarningMacro( "Unable to instantiate a database" );
    }

  vtkGenericWarningMacro( "Unable to connect to URL: " << URL );
  return db;
}

