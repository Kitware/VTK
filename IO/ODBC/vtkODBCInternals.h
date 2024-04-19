// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

VTK_ABI_NAMESPACE_BEGIN
class vtkODBCInternals
{
  friend class vtkODBCDatabase;
  friend class vtkODBCQuery;

public:
  vtkODBCInternals()
    : Environment(nullptr)
    , Connection(nullptr)
  {
  }

private:
  SQLHANDLE Environment;
  SQLHANDLE Connection;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkODBCInternals.h
