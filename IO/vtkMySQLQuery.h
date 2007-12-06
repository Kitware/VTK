/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMySQLQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMySQLQuery - vtkSQLQuery implementation for MySQL databases
//
// .SECTION Description
//
// This is an implementation of vtkSQLQuery for MySQL databases.  See
// the documentation for vtkSQLQuery for information about what the
// methods do.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkMySQLDatabase

#ifndef __vtkMySQLQuery_h
#define __vtkMySQLQuery_h

#include "vtkSQLQuery.h"

class vtkMySQLDatabase;
class vtkVariant;
class vtkVariantArray;
class vtkMySQLQueryInternals;

class VTK_IO_EXPORT vtkMySQLQuery : public vtkSQLQuery
{
//BTX
  friend class vtkMySQLDatabase;
//ETX

public:
  vtkTypeRevisionMacro(vtkMySQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMySQLQuery *New();

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


protected:
  vtkMySQLQuery();
  ~vtkMySQLQuery();

  vtkSetStringMacro(LastErrorText);

private:
  vtkMySQLQuery(const vtkMySQLQuery &); // Not implemented.
  void operator=(const vtkMySQLQuery &); // Not implemented.

  vtkMySQLQueryInternals *Internals;
  bool InitialFetch;
  char *LastErrorText;
};

#endif // __vtkMySQLQuery_h

