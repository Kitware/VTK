/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkODBCInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkODBCInternals
 * @brief   Simple class to hide ODBC structures
 *
 *
 *
 * There is no .cxx file to go along with this header.  Its sole
 * purpose is to let vtkODBCQuery and vtkODBCDatabase share the
 * variables that describe a single connection.
 *
 * @sa
 * vtkODBCDatabase vtkODBCQuery
 *
*/

#ifndef vtkODBCInternals_h
#define vtkODBCInternals_h

#include <sql.h>

class vtkODBCInternals {
  friend class vtkODBCDatabase;
  friend class vtkODBCQuery;

public:
  vtkODBCInternals()
    : Environment(0), Connection(0)
  {
  };

private:
  SQLHANDLE Environment;
  SQLHANDLE Connection;
};

#endif
// VTK-HeaderTest-Exclude: vtkODBCInternals.h
