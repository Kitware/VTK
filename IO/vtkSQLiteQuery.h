/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkSQLiteQuery - vtkSQLQuery implementation for SQLite databases
//
// .SECTION Description
//
// This is an implementation of vtkSQLQuery for SQLite databases.  See
// the documentation for vtkSQLQuery for information about what the
// methods do.
//
// .SECTION Bugs
//
// Sometimes Execute() will return false (meaning an error) but
// GetLastErrorText() winds up null.  I am not certain why this is
// happening.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this class.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkSQLiteDatabase

#ifndef __vtkSQLiteQuery_h
#define __vtkSQLiteQuery_h

#include "vtkSQLQuery.h"

class vtkSQLiteDatabase;
class vtkVariant;
class vtkVariantArray;
struct vtk_sqlite3_stmt;

class VTK_IO_EXPORT vtkSQLiteQuery : public vtkSQLQuery
{
  //BTX
  friend class vtkSQLiteDatabase;
  //ETX

public:
  vtkTypeRevisionMacro(vtkSQLiteQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLiteQuery *New();

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

  // Description:
  // Begin, abort (roll back), or commit a transaction.
  bool BeginTransaction();
  bool RollbackTransaction();
  bool CommitTransaction();

  //BTX
  // Description:
  // Return data in current row, field c
  vtkVariant DataValue(vtkIdType c);
  //ETX

  // Description:
  // Get the last error text from the query
  const char* GetLastErrorText();

protected:
  vtkSQLiteQuery();
  ~vtkSQLiteQuery();

  vtkSetStringMacro(LastErrorText);

private:
  vtkSQLiteQuery(const vtkSQLiteQuery &); // Not implemented.
  void operator=(const vtkSQLiteQuery &); // Not implemented.

  vtk_sqlite3_stmt *Statement;
  bool InitialFetch;
  int InitialFetchResult;
  char *LastErrorText;
  bool TransactionInProgress;
};

#endif // __vtkSQLiteQuery_h

