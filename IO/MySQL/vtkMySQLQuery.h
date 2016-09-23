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
/**
 * @class   vtkMySQLQuery
 * @brief   vtkSQLQuery implementation for MySQL databases
 *
 *
 *
 * This is an implementation of vtkSQLQuery for MySQL databases.  See
 * the documentation for vtkSQLQuery for information about what the
 * methods do.
 *
 *
 * @bug
 * Since MySQL requires that all bound parameters be passed in a
 * single mysql_stmt_bind_param call, there is no way to determine
 * which one is causing an error when one occurs.
 *
 *
 * @sa
 * vtkSQLDatabase vtkSQLQuery vtkMySQLDatabase
*/

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

  friend class vtkMySQLDatabase;

public:
  vtkTypeMacro(vtkMySQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMySQLQuery *New();

  /**
   * Set the SQL query string.  This must be performed before
   * Execute() or BindParameter() can be called.
   */
  bool SetQuery(const char *query);

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  bool Execute();

  //@{
  /**
   * Begin, commit, or roll back a transaction.

   * Calling any of these methods will overwrite the current query text
   * and call Execute() so any previous query text and results will be lost.
   */
  virtual bool BeginTransaction();
  virtual bool CommitTransaction();
  virtual bool RollbackTransaction();
  //@}

  /**
   * The number of fields in the query result.
   */
  int GetNumberOfFields();

  /**
   * Return the name of the specified query field.
   */
  const char* GetFieldName(int i);

  /**
   * Return the type of the field, using the constants defined in vtkType.h.
   */
  int GetFieldType(int i);

  /**
   * Advance row, return false if past end.
   */
  bool NextRow();

  /**
   * Return true if there is an error on the current query.
   */
  bool HasError();

  /**
   * Return data in current row, field c
   */
  vtkVariant DataValue(vtkIdType c);

  /**
   * Get the last error text from the query
   */
  const char* GetLastErrorText();

  /**
   * The following methods bind a parameter value to a placeholder in
   * the SQL string.  See the documentation for vtkSQLQuery for
   * further explanation.  The driver makes internal copies of string
   * and BLOB parameters so you don't need to worry about keeping them
   * in scope until the query finishes executing.
   */

  using vtkSQLQuery::BindParameter;
  bool BindParameter(int index, unsigned char value);
  bool BindParameter(int index, signed char value);
  bool BindParameter(int index, unsigned short value);
  bool BindParameter(int index, signed short value);
  bool BindParameter(int index, unsigned int value);

  bool BindParameter(int index, int value);

  bool BindParameter(int index, unsigned long value);
  bool BindParameter(int index, signed long value);
  bool BindParameter(int index, unsigned long long value);
  bool BindParameter(int index, long long value);

  bool BindParameter(int index, float value);
  bool BindParameter(int index, double value);
  /**
   * Bind a string value -- string must be null-terminated
   */
  bool BindParameter(int index, const char *stringValue);
  //@{
  /**
   * Bind a string value by specifying an array and a size
   */
  bool BindParameter(int index, const char *stringValue, size_t length);
  bool BindParameter(int index, const vtkStdString &string);
  //@}

  //@{
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check vtkSQLDatabase::IsSupported(VTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  bool BindParameter(int index, const void *data, size_t length);
  bool ClearParameterBindings();
  //@}

  /**
   * Escape a string for use in a query
   */
  virtual vtkStdString EscapeString( vtkStdString src, bool addSurroundingQuotes = true );

protected:
  vtkMySQLQuery();
  ~vtkMySQLQuery();

  vtkSetStringMacro(LastErrorText);

private:
  vtkMySQLQuery(const vtkMySQLQuery &) VTK_DELETE_FUNCTION;
  void operator=(const vtkMySQLQuery &) VTK_DELETE_FUNCTION;

  vtkMySQLQueryInternals *Internals;
  bool InitialFetch;
  char *LastErrorText;
};

#endif // vtkMySQLQuery_h

