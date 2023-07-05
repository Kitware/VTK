// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMySQLDatabasePrivate_h
#define vtkMySQLDatabasePrivate_h

#ifdef _WIN32
#include <winsock.h> // mysql.h relies on the typedefs from here
#endif

#include "vtkIOMySQLModule.h" // For export macro
#include <mysql.h>            // needed for MYSQL typedefs

VTK_ABI_NAMESPACE_BEGIN
class VTKIOMYSQL_EXPORT vtkMySQLDatabasePrivate
{
public:
  vtkMySQLDatabasePrivate()
    : Connection(nullptr)
  {
    mysql_init(&this->NullConnection);
  }

  MYSQL NullConnection;
  MYSQL* Connection;
};

VTK_ABI_NAMESPACE_END
#endif // vtkMySQLDatabasePrivate_h
// VTK-HeaderTest-Exclude: vtkMySQLDatabasePrivate.h
