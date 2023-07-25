// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMySQLDatabase.h"

#include <vtksys/SystemTools.hxx>

#include <string>

// Registration of MySQL dynamically with the vtkSQLDatabase factory method.
VTK_ABI_NAMESPACE_BEGIN
vtkSQLDatabase* MySQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  vtkMySQLDatabase* db = nullptr;

  if (vtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "mysql")
  {
    db = vtkMySQLDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int vtkIOMySQLCount;

VTKIOMYSQL_EXPORT void vtkIOMySQL_AutoInit_Construct()
{
  if (++vtkIOMySQLCount == 1)
  {
    vtkSQLDatabase::RegisterCreateFromURLCallback(MySQLCreateFunction);
  }
}
VTK_ABI_NAMESPACE_END
