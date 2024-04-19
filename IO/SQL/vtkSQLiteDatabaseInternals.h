// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
