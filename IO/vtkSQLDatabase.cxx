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

vtkCxxRevisionMacro(vtkSQLDatabase, "1.6");

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

  os << indent << "URL: " << this->URL  << endl;
}

// ----------------------------------------------------------------------
vtkSQLDatabase* vtkSQLDatabase::CreateFromURL( const char* URL )
{
  vtkstd::string protocol;
  vtkstd::string dataglom;

  if ( ! vtksys::SystemTools::ParseURLProtocol( URL, protocol, dataglom ) )
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return 0;
    }
  
  vtkSQLDatabase* db = 0;
  if ( protocol == "sqlite" )
    {
    db = vtkSQLiteDatabase::New();
    }
#ifdef VTK_USE_POSTGRES
  else if ( protocol == "psql" )
    {
    db = vtkPostgreSQLDatabase::New();
    }
#endif // VTK_USE_POSTGRES
#ifdef VTK_USE_MYSQL
  else if ( protocol == "mysql" )
    {
    db = vtkMySQLDatabase::New();
    }
#endif // VTK_USE_MYSQL
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
    vtkGenericWarningMacro( "Unable to instantiate a database with URL: " << URL );
    }

  return db;
}

