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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkPostgreSQLQuery - vtkSQLQuery implementation for PostgreSQL databases
//
// .SECTION Description
//
// This is an implementation of vtkSQLQuery for PostgreSQL databases.  See
// the documentation for vtkSQLQuery for information about what the
// methods do.
//
// .SECTION Thanks
//
// Thanks to David Thompson and Andy Wilson from Sandia National
// Laboratories for implementing this class.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkPostgreSQLDatabase

#ifndef __vtkPostgreSQLQuery_h
#define __vtkPostgreSQLQuery_h

#include "vtkIOPostgreSQLModule.h" // For export macro
#include "vtkSQLQuery.h"

class vtkPostgreSQLDatabase;
class vtkVariant;
class vtkVariantArray;
class vtkPostgreSQLQueryPrivate;

class VTKIOPOSTGRESQL_EXPORT vtkPostgreSQLQuery : public vtkSQLQuery
{
public:
  static vtkPostgreSQLQuery* New();
  void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkPostgreSQLQuery, vtkSQLQuery);

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

  // Description:
  // Return data in current row, field c
  vtkVariant DataValue( vtkIdType c );

  // Description:
  // Get the last error text from the query
  const char* GetLastErrorText();

  // Description:
  // Escape a string for inclusion into an SQL query
  virtual vtkStdString EscapeString( vtkStdString s, bool addSurroundingQuotes = true );

  // Description:
  // Unlike some databases, Postgres can tell you right away how many
  // rows are in the results of your query.
  int GetNumberOfRows();

protected:
  vtkPostgreSQLQuery();
  ~vtkPostgreSQLQuery();

  vtkSetStringMacro(LastErrorText);

  bool IsColumnBinary(int whichColumn);
  const char *GetColumnRawData(int whichColumn);

  bool TransactionInProgress;
  char *LastErrorText;
  int CurrentRow;

  vtkPostgreSQLQueryPrivate *QueryInternals;

  void DeleteQueryResults();

  //BTX
  friend class vtkPostgreSQLDatabase;
  //ETX

private:
  vtkPostgreSQLQuery( const vtkPostgreSQLQuery& ); // Not implemented.
  void operator = ( const vtkPostgreSQLQuery& ); // Not implemented.
};

#endif // __vtkPostgreSQLQuery_h

