// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLiteDatabase;
class vtkVariant;
class vtkVariantArray;

class VTKIOSQL_EXPORT vtkSQLiteQuery : public vtkSQLQuery
{

  friend class vtkSQLiteDatabase;

public:
  vtkTypeMacro(vtkSQLiteQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSQLiteQuery* New();

  /**
   * Set the SQL query string.  This must be performed before
   * Execute() or BindParameter() can be called.
   */
  bool SetQuery(const char* query) override;

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  bool Execute() override;

  /**
   * The number of fields in the query result.
   */
  int GetNumberOfFields() override;

  /**
   * Return the name of the specified query field.
   */
  const char* GetFieldName(int i) override;

  /**
   * Return the type of the field, using the constants defined in vtkType.h.
   */
  int GetFieldType(int i) override;

  /**
   * Advance row, return false if past end.
   */
  bool NextRow() override;

  /**
   * Return true if there is an error on the current query.
   */
  bool HasError() override;

  ///@{
  /**
   * Begin, abort (roll back), or commit a transaction.
   */
  bool BeginTransaction() override;
  bool RollbackTransaction() override;
  bool CommitTransaction() override;
  ///@}

  /**
   * Return data in current row, field c
   */
  vtkVariant DataValue(vtkIdType c) override;

  /**
   * Get the last error text from the query
   */
  const char* GetLastErrorText() override;

  /**
   * The following methods bind a parameter value to a placeholder in
   * the SQL string.  See the documentation for vtkSQLQuery for
   * further explanation.  The driver makes internal copies of string
   * and BLOB parameters so you don't need to worry about keeping them
   * in scope until the query finishes executing.
   */

  using vtkSQLQuery::BindParameter;
  bool BindParameter(int index, unsigned char value) override;
  bool BindParameter(int index, signed char value) override;
  bool BindParameter(int index, unsigned short value) override;
  bool BindParameter(int index, short value) override;
  bool BindParameter(int index, unsigned int value) override;

  bool BindParameter(int index, int value) override;

  bool BindParameter(int index, unsigned long value) override;
  bool BindParameter(int index, long value) override;
  bool BindParameter(int index, unsigned long long value) override;
  bool BindParameter(int index, long long value) override;

  bool BindParameter(int index, float value) override;
  bool BindParameter(int index, double value) override;
  /**
   * Bind a string value -- string must be null-terminated
   */
  bool BindParameter(int index, const char* stringValue) override;
  /**
   * Bind a string value by specifying an array and a size
   */
  bool BindParameter(int index, const char* stringValue, size_t length) override;

  bool BindParameter(int index, const vtkStdString& string) override;

  bool BindParameter(int index, vtkVariant value) override;
  ///@{
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check vtkSQLDatabase::IsSupported(VTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  bool BindParameter(int index, const void* data, size_t length) override;
  bool ClearParameterBindings() override;
  ///@}

protected:
  vtkSQLiteQuery();
  ~vtkSQLiteQuery() override;

  vtkSetStringMacro(LastErrorText);

private:
  vtkSQLiteQuery(const vtkSQLiteQuery&) = delete;
  void operator=(const vtkSQLiteQuery&) = delete;

  class Priv;
  Priv* Private;
  bool InitialFetch;
  int InitialFetchResult;
  char* LastErrorText;
  bool TransactionInProgress;

  ///@{
  /**
   * All of the BindParameter calls fall through to these methods
   * where we actually talk to sqlite.  You don't need to call them directly.
   */
  bool BindIntegerParameter(int index, int value);
  bool BindDoubleParameter(int index, double value);
  bool BindInt64Parameter(int index, vtkTypeInt64 value);
  bool BindStringParameter(int index, const char* data, int length);
  bool BindBlobParameter(int index, const void* data, int length);
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif // vtkSQLiteQuery_h
