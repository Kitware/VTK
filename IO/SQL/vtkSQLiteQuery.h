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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkSQLiteQuery
 * @brief   vtkSQLQuery implementation for SQLite databases
 *
 *
 *
 * This is an implementation of vtkSQLQuery for SQLite databases.  See
 * the documentation for vtkSQLQuery for information about what the
 * methods do.
 *
 *
 * @bug
 * Sometimes Execute() will return false (meaning an error) but
 * GetLastErrorText() winds up null.  I am not certain why this is
 * happening.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for implementing
 * this class.
 *
 * @sa
 * vtkSQLDatabase vtkSQLQuery vtkSQLiteDatabase
*/

#ifndef vtkSQLiteQuery_h
#define vtkSQLiteQuery_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkSQLQuery.h"

class vtkSQLiteDatabase;
class vtkVariant;
class vtkVariantArray;
struct vtk_sqlite3_stmt;

class VTKIOSQL_EXPORT vtkSQLiteQuery : public vtkSQLQuery
{

  friend class vtkSQLiteDatabase;

public:
  vtkTypeMacro(vtkSQLiteQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLiteQuery *New();

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

  //@{
  /**
   * Begin, abort (roll back), or commit a transaction.
   */
  bool BeginTransaction();
  bool RollbackTransaction();
  bool CommitTransaction();
  //@}

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
  bool BindParameter(int index, short value);
  bool BindParameter(int index, unsigned int value);

  bool BindParameter(int index, int value);

  bool BindParameter(int index, unsigned long value);
  bool BindParameter(int index, long value);
  bool BindParameter(int index, unsigned long long value);
  bool BindParameter(int index, long long value);

  bool BindParameter(int index, float value);
  bool BindParameter(int index, double value);
  /**
   * Bind a string value -- string must be null-terminated
   */
  bool BindParameter(int index, const char *stringValue);
  /**
   * Bind a string value by specifying an array and a size
   */
  bool BindParameter(int index, const char *stringValue, size_t length);

  bool BindParameter(int index, const vtkStdString &string);

  bool BindParameter(int index, vtkVariant value);
  //@{
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check vtkSQLDatabase::IsSupported(VTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  bool BindParameter(int index, const void *data, size_t length);
  bool ClearParameterBindings();
  //@}

protected:
  vtkSQLiteQuery();
  ~vtkSQLiteQuery();

  vtkSetStringMacro(LastErrorText);

private:
  vtkSQLiteQuery(const vtkSQLiteQuery &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQLiteQuery &) VTK_DELETE_FUNCTION;

  vtk_sqlite3_stmt *Statement;
  bool InitialFetch;
  int InitialFetchResult;
  char *LastErrorText;
  bool TransactionInProgress;

  //@{
  /**
   * All of the BindParameter calls fall through to these methods
   * where we actually talk to sqlite.  You don't need to call them directly.
   */
  bool BindIntegerParameter(int index, int value);
  bool BindDoubleParameter(int index, double value);
  bool BindInt64Parameter(int index, vtkTypeInt64 value);
  bool BindStringParameter(int index, const char *data, int length);
  bool BindBlobParameter(int index, const void *data, int length);
  //@}

};

#endif // vtkSQLiteQuery_h

