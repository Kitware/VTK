/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteDatabaseInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSQLiteDatabaseInternals_h
#define vtkSQLiteDatabaseInternals_h

#include "vtk_sqlite.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLiteDatabaseInternals
{
public:
  sqlite3* SQLiteInstance;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkSQLiteDatabaseInternals.h
