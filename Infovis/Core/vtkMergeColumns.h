// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkMergeColumns
 * @brief   merge two columns into a single column
 *
 *
 * vtkMergeColumns replaces two columns in a table with a single column
 * containing data in both columns.  The columns are set using
 *
 *   SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "col1")
 *
 * and
 *
 *   SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "col2")
 *
 * where "col1" and "col2" are the names of the columns to merge.
 * The user may also specify the name of the merged column.
 * The arrays must be of the same type.
 * If the arrays are numeric, the values are summed in the merged column.
 * If the arrays are strings, the values are concatenated.  The strings are
 * separated by a space if they are both nonempty.
 */

#ifndef vtkMergeColumns_h
#define vtkMergeColumns_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkMergeColumns : public vtkTableAlgorithm
{
public:
  static vtkMergeColumns* New();
  vtkTypeMacro(vtkMergeColumns, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The name to give the merged column created by this filter.
   */
  vtkSetStringMacro(MergedColumnName);
  vtkGetStringMacro(MergedColumnName);
  ///@}

protected:
  vtkMergeColumns();
  ~vtkMergeColumns() override;

  char* MergedColumnName;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMergeColumns(const vtkMergeColumns&) = delete;
  void operator=(const vtkMergeColumns&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
