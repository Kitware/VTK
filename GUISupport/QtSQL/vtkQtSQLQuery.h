// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkQtSQLQuery
 * @brief   query class associated with vtkQtSQLDatabase
 *
 *
 * Implements vtkSQLQuery using an underlying QSQLQuery.
 */

#ifndef vtkQtSQLQuery_h
#define vtkQtSQLQuery_h

#include "vtkGUISupportQtSQLModule.h" // For export macro
#include "vtkSQLQuery.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkVariant;
class vtkQtSQLQueryInternals;

class VTKGUISUPPORTQTSQL_EXPORT vtkQtSQLQuery : public vtkSQLQuery
{
public:
  static vtkQtSQLQuery* New();
  vtkTypeMacro(vtkQtSQLQuery, vtkSQLQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  const char* GetFieldName(int col) override;

  /**
   * Return the type of the specified query field, as defined in vtkType.h.
   */
  int GetFieldType(int col) override;

  /**
   * Advance row, return false if past end.
   */
  bool NextRow() override;

  /**
   * Return data in current row, field c
   */
  vtkVariant DataValue(vtkIdType c) override;

  /**
   * Returns true if an error is set, otherwise false.
   */
  bool HasError() override;

  /**
   * Get the last error text from the query
   */
  const char* GetLastErrorText() override;

protected:
  vtkQtSQLQuery();
  ~vtkQtSQLQuery() override;

  vtkQtSQLQueryInternals* Internals;
  friend class vtkQtSQLDatabase;

private:
  // Using the convenience function internally
  vtkSetStringMacro(LastErrorText);

  char* LastErrorText;

  vtkQtSQLQuery(const vtkQtSQLQuery&) = delete;
  void operator=(const vtkQtSQLQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkQtSQLQuery_h
