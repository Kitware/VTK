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
// .SECTION Bugs
//
// Since MySQL requires that all bound parameters be passed in a
// single mysql_stmt_bind_param call, there is no way to determine
// which one is causing an error when one occurs.
//
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery vtkMySQLDatabase

#ifndef vtkMySQLQuery_h
#define vtkMySQLQuery_h

#include "vtkIOMySQLModule.h" // For export macro
#include "vtkSQLQuery.h"

class vtkMySQLDatabase;
class vtkVariant;
class vtkVariantArray;
class vtkMySQLQueryInternals;

class VTKIOMYSQL_EXPORT vtkMySQLQuery : public vtkSQLQuery
{
//BTX
  friend class vtkMySQLDatabase;
//ETX

public:
  vtkTypeMacro(vtkMySQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMySQLQuery *New();

  // Description:
  // Set the SQL query string.  This must be performed before
  // Execute() or BindParameter() can be called.
  bool SetQuery(const char *query);

  // Description:
  // Execute the query.  This must be performed
  // before any field name or data access functions
  // are used.
  bool Execute();

  // Description:
  // Begin, commit, or roll back a transaction.
  //
  // Calling any of these methods will overwrite the current query text
  // and call Execute() so any previous query text and results will be lost.
  virtual bool BeginTransaction();
  virtual bool CommitTransaction();
  virtual bool RollbackTransaction();

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
  // Return data in current row, field c
  vtkVariant DataValue(vtkIdType c);

  // Description:
  // Get the last error text from the query
  const char* GetLastErrorText();

  // Description:
  // The following methods bind a parameter value to a placeholder in
  // the SQL string.  See the documentation for vtkSQLQuery for
  // further explanation.  The driver makes internal copies of string
  // and BLOB parameters so you don't need to worry about keeping them
  // in scope until the query finishes executing.
//BTX
  using vtkSQLQuery::BindParameter;
  bool BindParameter(int index, unsigned char value);
  bool BindParameter(int index, signed char value);
  bool BindParameter(int index, unsigned short value);
  bool BindParameter(int index, signed short value);
  bool BindParameter(int index, unsigned int value);
//ETX
  bool BindParameter(int index, int value);
//BTX
  bool BindParameter(int index, unsigned long value);
  bool BindParameter(int index, signed long value);
  bool BindParameter(int index, vtkTypeUInt64 value);
  bool BindParameter(int index, vtkTypeInt64 value);
//ETX
  bool BindParameter(int index, float value);
  bool BindParameter(int index, double value);
  // Description:
  // Bind a string value -- string must be null-terminated
  bool BindParameter(int index, const char *stringValue);
  // Description:
  // Bind a string value by specifying an array and a size
  bool BindParameter(int index, const char *stringValue, size_t length);
  bool BindParameter(int index, const vtkStdString &string);

  // Description:
  // Bind a blob value.  Not all databases support blobs as a data
  // type.  Check vtkSQLDatabase::IsSupported(VTK_SQL_FEATURE_BLOB) to
  // make sure.
  bool BindParameter(int index, const void *data, size_t length);
  bool ClearParameterBindings();

  // Description:
  // Escape a string for use in a query
  virtual vtkStdString EscapeString( vtkStdString src, bool addSurroundingQuotes = true );

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

#endif // vtkMySQLQuery_h

