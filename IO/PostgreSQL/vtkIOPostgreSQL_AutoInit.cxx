/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOPostgreSQL_AutoInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkPostgreSQLDatabase.h"

#include <vtksys/SystemTools.hxx>

#include <string>

// Registration of PostgreSQL dynamically with the vtkSQLDatabase factory method.
vtkSQLDatabase * PostgreSQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  vtkPostgreSQLDatabase *db = 0;

  if (vtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) &&
      protocol == "psql")
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

VTKIOPOSTGRESQL_EXPORT void vtkIOPostgreSQL_AutoInit_Destruct()
{
  if (--vtkIOPostgreSQLCount == 0)
  {
    vtkSQLDatabase::UnRegisterCreateFromURLCallback(PostgreSQLCreateFunction);
  }
}
