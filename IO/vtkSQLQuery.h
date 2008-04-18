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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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
#include "vtkStdString.h" // for EscapeString()

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
  // Execute the query.  This must be performed
  // before any field name or data access functions
  // are used.
  virtual bool Execute() = 0;

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

  //BTX
  // Description:
  // Escape a string for inclusion into an SQL query.
  // If \a addSurroundingQuotes is true, then quotation marks appropriate to the
  // backend database will be added to enclose the escaped string. This argument
  // defaults to true.
  //
  // A default, simple-minded implementation is provided for
  // database backends that do not provde a way to escape
  // strings for use inside queries.
  virtual vtkStdString EscapeString( vtkStdString s, bool addSurroundingQuotes = true );
  //ETX

  // Description:
  // Escape a string for inclusion into an SQL query.
  // This method exists to provide a wrappable version of
  // the method that takes and returns vtkStdString objects.
  // You are responsible for calling delete [] on the
  // character array returned by this method.
  // This method simply calls the vtkStdString variant and thus
  // need not be re-implemented by subclasses.
  virtual char* EscapeString( const char* src, bool addSurroundingQuotes );

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

