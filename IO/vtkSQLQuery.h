/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLQuery.h

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
// .NAME vtkSQLQuery - executes an sql query and retrieves results
//
// .SECTION Description
// The abstract superclass of SQL query classes.  Instances of subclasses
// of vtkSQLQuery are created using the GetQueryInstance() function in
// vtkSQLDatabase.  To implement a query connection for a new database
// type, subclass both vtkSQLDatabase and vtkSQLQuery, and implement the
// required functions.  For the query class, this involves the following:
//
// Execute() - Execute the query on the database.  No results need to be
//             retrieved at this point, unless you are performing caching.
//
// GetNumberOfFields() - After Execute() is performed, returns the number
//                       of fields in the query results.
//
// GetFieldName() - The name of the field at an index.
//
// GetFieldType() - The data type of the field at an index.
//
// NextRow() - Advances the query results by one row, and returns whether
//             there are more rows left in the query.
//
// DataValue() - Extract a single data value from the current row.
//
// Begin/Rollback/CommitTransaction() - These methods are optional but
// recommended if the database supports transactions.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes.
//
// .SECTION See Also
// vtkSQLDatabase

#ifndef __vtkSQLQuery_h
#define __vtkSQLQuery_h

#include "vtkRowQuery.h"

class vtkSQLDatabase;
class vtkVariant;
class vtkVariantArray;

class VTK_IO_EXPORT vtkSQLQuery : public vtkRowQuery
{
public:
  vtkTypeRevisionMacro(vtkSQLQuery, vtkRowQuery);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The query string to be executed.
  vtkGetStringMacro(Query);
  vtkSetStringMacro(Query);

  // Description:
  // Return true if the query is active (i.e. execution was successful
  // and results are ready to be fetched).  Returns false on error or
  // inactive query.
  bool IsActive() { return this->Active; }

  // Description:
  // Begin, commit, or roll back a transaction.  If the underlying
  // database does not support transactions these calls will do
  // nothing.
  virtual bool BeginTransaction() { return true; }
  virtual bool CommitTransaction() { return true; }
  virtual bool RollbackTransaction() { return true; }

  // Description:
  // Return the database associated with the query.
  vtkGetObjectMacro(Database, vtkSQLDatabase);

protected:
  vtkSQLQuery();
  ~vtkSQLQuery();

  // Description:
  // Set the database associated with the query.
  // This is only to be called by the corresponding
  // database class on creation of the query in
  // GetQueryInstance().
  void SetDatabase(vtkSQLDatabase* db);

  char* Query;
  vtkSQLDatabase* Database;
  bool Active;

private:
  vtkSQLQuery(const vtkSQLQuery &); // Not implemented.
  void operator=(const vtkSQLQuery &); // Not implemented.
};

#endif // __vtkSQLQuery_h

