// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSumTables
 * @brief   Matrix-like summation of tables.
 *
 * vtkSumTables is a filter that operates on vtkTable objects to perform a summation
 * operation on each table entry.
 * The tables must have the same column names and types, must have the same number of
 * rows, and all columns must be data arrays (because string and variant columns do
 * not support mathematical summation).
 *
 * This class also exposes a simple static method for in-place summation.
 */

#ifndef vtkSumTables_h
#define vtkSumTables_h

#include "vtkDataArray.h"            // For numeric key columns
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStringArray.h"          // For string key columns
#include "vtkTable.h"                // For table inputs
#include "vtkTableAlgorithm.h"

#include <map>    // For left and right key maps
#include <string> // For LeftKey and RightKey

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSGENERAL_EXPORT vtkSumTables : public vtkTableAlgorithm
{
public:
  static vtkSumTables* New();
  vtkTypeMacro(vtkSumTables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set a pipeline connection on port 1 for the right table. This method is equivalent
   * to SetInputConnection(1, source).
   */
  void SetSourceConnection(vtkAlgorithmOutput* source);

  /**
   * Specify input data on port 1 for the right table. This method is equivalent
   * to SetInputData(1, source).
   */
  void SetSourceData(vtkTable* source);

  /// Sum tables \a aa and \a bb, storing the result in \a aa.
  ///
  /// If \a checkOnly is true, then the tables will be tested
  /// for compatibility but no sum will be computed (i.e., table \a aa
  /// will not be altered). The default is false.
  ///
  /// If \a allowAbstractColumns is true, then string- or variant-arrays
  /// will be ignored during summation. (Thus \a aa's existing values for
  /// these columns will be ignored.) The default is false.
  static bool SumTables(
    vtkTable* aa, vtkTable* bb, bool checkOnly = false, bool allowAbstractColumns = false);

protected:
  vtkSumTables();
  ~vtkSumTables() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkSumTables(const vtkSumTables&) = delete;
  void operator=(const vtkSumTables&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkSumTables_h
