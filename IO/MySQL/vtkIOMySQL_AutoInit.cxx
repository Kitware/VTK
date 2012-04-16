/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOMySQL_AutoInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMySQLDatabase.h"

#include <vtksys/SystemTools.hxx>

#include <string>

// Registration of MySQL dynamically with the vtkSQLDatabase factory method.
vtkSQLDatabase * MySQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  vtkMySQLDatabase *db = 0;

  if (vtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) &&
      protocol == "mysql")
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

VTKIOMYSQL_EXPORT void vtkIOMySQL_AutoInit_Destruct()
{
  if (--vtkIOMySQLCount == 0)
    {
    vtkSQLDatabase::UnRegisterCreateFromURLCallback(MySQLCreateFunction);
    }
}
