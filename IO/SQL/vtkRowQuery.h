// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRowQuery
 * @brief   abstract interface for queries that return
 * row-oriented results.
 *
 *
 * The abstract superclass of query classes that return row-oriented (table)
 * results.  A subclass will provide database-specific query parameters and
 * implement the vtkRowQuery API to return query results:
 *
 * Execute() - Execute the query.  No results need to be retrieved at this
 *             point, unless you are performing caching.
 *
 * GetNumberOfFields() - After Execute() is performed, returns the number
 *                       of fields in the query results.
 *
 * GetFieldName() - The name of the field at an index.
 *
 * GetFieldType() - The data type of the field at an index.
 *
 * NextRow() - Advances the query results by one row, and returns whether
 *             there are more rows left in the query.
 *
 * DataValue() - Extract a single data value from the current row.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for his work
 * on the database classes.
 *
 * @sa
 * vtkRowQueryToTable
 */

#ifndef vtkRowQuery_h
#define vtkRowQuery_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkVariant;
class vtkVariantArray;

class VTKIOSQL_EXPORT vtkRowQuery : public vtkObject
{
public:
  vtkTypeMacro(vtkRowQuery, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  virtual bool Execute() = 0;

  /**
   * The number of fields in the query result.
   */
  virtual int GetNumberOfFields() = 0;

  /**
   * Return the name of the specified query field.
   */
  virtual const char* GetFieldName(int i) = 0;

  /**
   * Return the type of the field, using the constants defined in vtkType.h.
   */
  virtual int GetFieldType(int i) = 0;

  /**
   * Return the index of the specified query field.
   * Uses GetNumberOfFields() and GetFieldName()
   * to match field name.
   */
  int GetFieldIndex(const char* name);

  /**
   * Advance row, return false if past end.
   */
  virtual bool NextRow() = 0;

  /**
   * Return true if the query is active (i.e. execution was successful
   * and results are ready to be fetched).  Returns false on error or
   * inactive query.
   */
  virtual bool IsActive() = 0;

  /**
   * Advance row, return false if past end.
   * Also, fill array with row values.
   */
  bool NextRow(vtkVariantArray* rowArray);

  /**
   * Return data in current row, field c
   */
  virtual vtkVariant DataValue(vtkIdType c) = 0;

  /**
   * Returns true if an error is set, otherwise false.
   */
  virtual bool HasError() = 0;

  /**
   * Get the last error text from the query
   */
  virtual const char* GetLastErrorText() = 0;

  ///@{
  /**
   * Many databases do not preserve case in field names.  This can
   * cause GetFieldIndex to fail if you search for a field named
   * someFieldName when the database actually stores it as
   * SOMEFIELDNAME.  This ivar controls whether GetFieldIndex()
   * expects field names to be case-sensitive.  The default is OFF,
   * i.e. case is not preserved.
   */
  vtkSetMacro(CaseSensitiveFieldNames, bool);
  vtkGetMacro(CaseSensitiveFieldNames, bool);
  vtkBooleanMacro(CaseSensitiveFieldNames, bool);
  ///@}

protected:
  vtkRowQuery();
  ~vtkRowQuery() override;
  bool CaseSensitiveFieldNames;

private:
  vtkRowQuery(const vtkRowQuery&) = delete;
  void operator=(const vtkRowQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkRowQuery_h
