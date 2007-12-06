/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostgreSQLQuery.h

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
// .NAME vtkPostgreSQLQuery - vtkSQLQuery implementation for PostgreSQL databases
//
// .SECTION Description
//
// This is an implementation of vtkSQLQuery for PostgreSQL databases.  See
// the documentation for vtkSQLQuery for information about what the
// methods do.
//
// .SECTION Bugs
//
// The PostgreSQL adaptor classes use pqxx instead of libpq.
//
// .SECTION Thanks
// Thanks to David Thompson from Sandia National Laboratories for implementing
// this class. Thanks to Andrew Wilson for the SQLite example.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkPostgreSQLDatabase

#ifndef __vtkPostgreSQLQuery_h
#define __vtkPostgreSQLQuery_h

#include "vtkSQLQuery.h"

class vtkPostgreSQLDatabase;
class vtkVariant;
class vtkVariantArray;
class vtkPostgreSQLQueryPrivate;

class VTK_IO_EXPORT vtkPostgreSQLQuery : public vtkSQLQuery
{
  //BTX
  friend class vtkPostgreSQLDatabase;
  //ETX

public:
  static vtkPostgreSQLQuery* New();
  void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkPostgreSQLQuery, vtkSQLQuery);

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
  const char* GetFieldName( int i );

  // Description:
  // Return the type of the field, using the constants defined in vtkType.h.
  int GetFieldType( int i );

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
  vtkVariant DataValue( vtkIdType c );
  //ETX

  // Description:
  // Get the last error text from the query
  const char* GetLastErrorText();

protected:
  vtkPostgreSQLQuery();
  ~vtkPostgreSQLQuery();

  void SetLastErrorText( const char* msg );

  vtkPostgreSQLQueryPrivate* Transactor;

private:
  vtkPostgreSQLQuery( const vtkPostgreSQLQuery& ); // Not implemented.
  void operator = ( const vtkPostgreSQLQuery& ); // Not implemented.
};

#endif // __vtkPostgreSQLQuery_h

