/* -*- Mode: C++; -*- */
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkODBCQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkODBCQuery - vtkSQLQuery implementation for ODBC connections to databases
//
// .SECTION Description
//
// This is an implementation of vtkSQLQuery for ODBC databases.  See
// the documentation for vtkSQLQuery and vtkRowQuery for information
// about what the methods do.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkODBCDatabase

#ifndef __vtkODBCQuery_h
#define __vtkODBCQuery_h

#include "vtkSQLQuery.h"

class vtkODBCDatabase;
class vtkVariant;
class vtkVariantArray;
class vtkODBCInternals;
class vtkODBCQueryInternals;

class VTK_IO_EXPORT vtkODBCQuery : public vtkSQLQuery
{
//BTX
  friend class vtkODBCDatabase;
//ETX

public:
  vtkTypeRevisionMacro(vtkODBCQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkODBCQuery *New();

  // Description:
  // Execute the query.  This must be performed
  // before any field name or data access functions
  // are used.
  bool Execute();

  // Description:
  // The number of fields in the query result.
  int GetNumberOfFields();

  // Description:
  // Return the name of the specified query field.
  const char* GetFieldName(int i);

  // Description:
  // Return the type of the field, using the constants defined in vtkType.h.
  int GetFieldType(int i);

  // Description:
  // Advance row, return false if past end.
  bool NextRow();

  // Description:
  // Return true if there is an error on the current query.
  bool HasError();

//BTX
  // Description:
  // Return data in current row, field c
  vtkVariant DataValue(vtkIdType c);
//ETX

  // Description:
  // Get the last error text from the query
  const char* GetLastErrorText();

  // Description:
  // Begin, commit, or roll back a transaction.  
  bool BeginTransaction();
  bool CommitTransaction();
  bool RollbackTransaction();

protected:
  vtkODBCQuery();
  ~vtkODBCQuery();

  vtkSetStringMacro(LastErrorText);

private:
  vtkODBCQuery(const vtkODBCQuery &); // Not implemented.
  void operator=(const vtkODBCQuery &); // Not implemented.

  void ClearCurrentRow();
  bool CacheCurrentRow();

  bool CacheTimeColumn(int column);
  bool CacheIntervalColumn(int column);
  bool CacheCharColumn(int column);
  bool CacheLongLongColumn(int column);
  bool CacheBinaryColumn(int column);
  bool CacheBooleanColumn(int column);
  bool CacheStringColumn(int column);
  bool CacheWideStringColumn(int column);
  bool CacheDecimalColumn(int column);
  bool CacheNumericColumn(int column);
  bool CacheIntColumn(int column);
  bool CacheFloatColumn(int column);
  bool CacheDoubleColumn(int column);

  vtkODBCQueryInternals *Internals;
  bool InitialFetch;
  char *LastErrorText;
};

#endif // __vtkODBCQuery_h

