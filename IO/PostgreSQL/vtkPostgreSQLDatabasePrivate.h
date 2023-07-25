// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkPostgreSQLDatabasePrivate
 * @brief   internal details of a connection to a PostgreSQL database
 *
 *
 *
 * This class does two things.  First, it holds the (pointer to the)
 * PGconn struct that represents an actual database connection.  Second,
 * it holds a map from Postgres data types as they exist in the database
 * to VTK data types.
 *
 * You should never have to deal with this class outside of
 * vtkPostgreSQLDatabase and vtkPostgreSQLQuery.
 */

#ifndef vtkPostgreSQLDatabasePrivate_h
#define vtkPostgreSQLDatabasePrivate_h

#include "vtkTimeStamp.h"
#include "vtkType.h"

#include <libpq-fe.h>
#include <map>

VTK_ABI_NAMESPACE_BEGIN
class vtkPostgreSQLDatabasePrivate
{
public:
  vtkPostgreSQLDatabasePrivate() { this->Connection = nullptr; }

  /**
   * Destroy the database connection. Any uncommitted transaction will be aborted.
   */
  virtual ~vtkPostgreSQLDatabasePrivate()
  {
    if (this->Connection)
    {
      PQfinish(this->Connection);
    }
  }

  // Given a Postgres column type OID, return a VTK array type (see vtkType.h).
  int GetVTKTypeFromOID(Oid pgtype)
  {
    std::map<Oid, int>::const_iterator it = this->DataTypeMap.find(pgtype);
    if (it == this->DataTypeMap.end())
    {
      return VTK_STRING;
    }
    return it->second;
  }

  // This is the actual database connection.  It will be nullptr if no
  // connection is open.
  PGconn* Connection;

  // Map Postgres column types to VTK types.
  std::map<Oid, int> DataTypeMap;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPostgreSQLDatabasePrivate_h
// VTK-HeaderTest-Exclude: vtkPostgreSQLDatabasePrivate.h
