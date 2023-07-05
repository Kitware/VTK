// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkODBCDatabase.h"

#include <vtksys/SystemTools.hxx>

#include <string>

// Registration of ODBC dynamically with the vtkSQLDatabase factory method.
VTK_ABI_NAMESPACE_BEGIN
vtkSQLDatabase* ODBCCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  vtkODBCDatabase* db = nullptr;

  if (vtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "odbc")
  {
    db = vtkODBCDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int vtkIOODBCCount;

VTKIOODBC_EXPORT void vtkIOODBC_AutoInit_Construct()
{
  if (++vtkIOODBCCount == 1)
  {
    vtkSQLDatabase::RegisterCreateFromURLCallback(ODBCCreateFunction);
  }
}
VTK_ABI_NAMESPACE_END
