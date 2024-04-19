// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkAdjacencyMatrixToEdgeTable
 *
 *
 * Treats a dense 2-way array of doubles as an adacency matrix and converts it into a
 * vtkTable suitable for use as an edge table with vtkTableToGraph.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkAdjacencyMatrixToEdgeTable_h
#define vtkAdjacencyMatrixToEdgeTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkAdjacencyMatrixToEdgeTable : public vtkTableAlgorithm
{
public:
  static vtkAdjacencyMatrixToEdgeTable* New();
  vtkTypeMacro(vtkAdjacencyMatrixToEdgeTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specifies whether rows or columns become the "source" in the output edge table.
   * 0 = rows, 1 = columns.  Default: 0
   */
  vtkGetMacro(SourceDimension, vtkIdType);
  vtkSetMacro(SourceDimension, vtkIdType);
  ///@}

  ///@{
  /**
   * Controls the name of the output table column that contains edge weights.
   * Default: "value"
   */
  vtkGetStringMacro(ValueArrayName);
  vtkSetStringMacro(ValueArrayName);
  ///@}

  ///@{
  /**
   * Specifies the minimum number of adjacent edges to include for each source vertex.
   * Default: 0
   */
  vtkGetMacro(MinimumCount, vtkIdType);
  vtkSetMacro(MinimumCount, vtkIdType);
  ///@}

  ///@{
  /**
   * Specifies a minimum threshold that an edge weight must exceed to be included in
   * the output.
   * Default: 0.5
   */
  vtkGetMacro(MinimumThreshold, double);
  vtkSetMacro(MinimumThreshold, double);
  ///@}

protected:
  vtkAdjacencyMatrixToEdgeTable();
  ~vtkAdjacencyMatrixToEdgeTable() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkIdType SourceDimension;
  char* ValueArrayName;
  vtkIdType MinimumCount;
  double MinimumThreshold;

private:
  vtkAdjacencyMatrixToEdgeTable(const vtkAdjacencyMatrixToEdgeTable&) = delete;
  void operator=(const vtkAdjacencyMatrixToEdgeTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
