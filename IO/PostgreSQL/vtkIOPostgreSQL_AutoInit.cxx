// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPostgreSQLDatabase.h"

#include <vtksys/SystemTools.hxx>

#include <string>

// Registration of PostgreSQL dynamically with the vtkSQLDatabase factory method.
VTK_ABI_NAMESPACE_BEGIN
vtkSQLDatabase* PostgreSQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  vtkPostgreSQLDatabase* db = nullptr;

  if (vtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "psql")
  {
    db = vtkPostgreSQLDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int vtkIOPostgreSQLCount;

struct VTKIOPOSTGRESQL_EXPORT vtkIOPostgreSQL_AutoInit
{
  vtkIOPostgreSQL_AutoInit();
  ~vtkIOPostgreSQL_AutoInit();
};

VTKIOPOSTGRESQL_EXPORT void vtkIOPostgreSQL_AutoInit_Construct()
{
  if (++vtkIOPostgreSQLCount == 1)
  {
    vtkSQLDatabase::RegisterCreateFromURLCallback(PostgreSQLCreateFunction);
  }
}
VTK_ABI_NAMESPACE_END
