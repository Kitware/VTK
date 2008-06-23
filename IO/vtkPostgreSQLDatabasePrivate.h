/* -*- Mode: C++; -*- */
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostgreSQLDatabasePrivate.h

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

// .NAME vtkPostgreSQLDatabasePrivate - internal details of a connection to a PostgreSQL database
//
// .SECTION Description
//
// This class does two things.  First, it holds the (pointer to the)
// PGconn struct that represents an actual database connection.  Second,
// it holds a map from Postgres data types as they exist in the database
// to VTK data types.
//
// You should never have to deal with this class outside of
// vtkPostgreSQLDatabase and vtkPostgreSQLQuery.

#ifndef __vtkPostgreSQLDatabasePrivate_h
#define __vtkPostgreSQLDatabasePrivate_h

#include "vtkStdString.h"
#include "vtkType.h"
#include "vtkTimeStamp.h"

#include <libpq-fe.h>
#include <vtksys/stl/map>

class vtkPostgreSQLDatabasePrivate
{
 public:
  vtkPostgreSQLDatabasePrivate() 
    {
      this->Connection = NULL;
    }
    
  // Description:
  // Destroy the database connection. Any uncommitted transaction will be aborted.
    virtual ~vtkPostgreSQLDatabasePrivate()
      {
      if (this->Connection)
        {
        PQfinish(this->Connection);
        }
      }

    // Given a Postgres column type OID, return a VTK array type (see vtkType.h).
    int GetVTKTypeFromOID( Oid pgtype )
    {
      vtksys_stl::map<Oid,int>::const_iterator it = this->DataTypeMap.find( pgtype );
      if ( it == this->DataTypeMap.end() )
        {
        return VTK_STRING;
        }
      return it->second;
    }


  // Description:

  // This is the actual database connection.  It will be NULL if no
  // connection is open.
  PGconn  *Connection;

  // Map Postgres column types to VTK types.
  vtksys_stl::map<Oid,int> DataTypeMap;
};

#endif // __vtkPostgreSQLDatabasePrivate_h
