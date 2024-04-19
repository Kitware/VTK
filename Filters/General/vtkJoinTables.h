// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJoinTables
 * @brief   SQL-style Join operation on two tables.
 *
 *
 * vtkJoinTables is a filter that operates on two vtkTable objects to perform an
 * SQL-style Join operation. It outputs one vtkTable object. The goal is to combine the rows of both
 * tables into one bigger table based on a related column between them (both inputs have their "key
 * column"). The two input tables are referred to as left and right. In each input table, the values
 * in the key column act like unique IDs for their respective  rows. During the merge, the
 * attributes of each item will be given with respect to its ID.
 */

#ifndef vtkJoinTables_h
#define vtkJoinTables_h

#include "vtkDataArray.h"            // For numeric key columns
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStringArray.h"          // For string key columns
#include "vtkTable.h"                // For table inputs
#include "vtkTableAlgorithm.h"

#include <map>    // For left and right key maps
#include <string> // For LeftKey and RightKey

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkJoinTables : public vtkTableAlgorithm
{
public:
  static vtkJoinTables* New();
  vtkTypeMacro(vtkJoinTables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum JoinMode
  {
    INTERSECTION = 0,
    UNION = 1,
    LEFT = 2,
    RIGHT = 3
  };

  ///@{
  /**
   * The mode of the Join Tables filter. This is meaningful when the two key columns do not share
   * exactly the same set of values. The different Join modes that this filter implements are :
   * - INTERSECTION : Keeps only the keys that are in both columns.
   * - UNION : Keeps all of the keys from both tables.
   * - LEFT : Keeps the keys from the left table.
   * - RIGHT : Keeps the keys from the right table.
   */
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);
  ///@}

  ///@{
  /**
   * Value to be imputed in numeric columns of the output when the data for a given key in a given
   * column is unknown.
   */
  vtkSetMacro(ReplacementValue, double);
  vtkGetMacro(ReplacementValue, double);
  ///@}

  ///@{
  /**
   * Specifies which column of the left table to use for the join operation.
   */
  vtkSetStdStringFromCharMacro(LeftKey);
  vtkGetCharFromStdStringMacro(LeftKey);
  ///@}

  ///@{
  /**
   * Specifies which column of the right table to use for the join operation.
   */
  vtkSetStdStringFromCharMacro(RightKey);
  vtkGetCharFromStdStringMacro(RightKey);
  ///@}

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

protected:
  vtkJoinTables();
  ~vtkJoinTables() override = default;

  template <typename T>
  struct Maps
  {
    std::map<T, int> left;
    std::map<T, int> right;
  };

  template <typename ColType, typename KeyColType, typename KeyValues>
  void MergeColumn(ColType*, ColType*, KeyColType*, const char*, std::map<KeyValues, int>);

  template <typename KeyColType, typename KeyValues>
  void JoinAlgorithm(vtkTable*, vtkTable*, vtkTable*, KeyColType*, KeyColType*, Maps<KeyValues>*);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int Mode = JoinMode::INTERSECTION;
  std::string LeftKey;
  std::string RightKey;
  double ReplacementValue = 0;

private:
  vtkJoinTables(const vtkJoinTables&) = delete;
  void operator=(const vtkJoinTables&) = delete;
};

VTK_ABI_NAMESPACE_END
#include "vtkJoinTables.txx" // for template implementations

#endif
