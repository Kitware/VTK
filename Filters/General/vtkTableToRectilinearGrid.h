// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableToRectilinearGrid
 * @brief   Converts vtkTable to a vtkRectilinearGrid.
 *
 * vtkTableToRectilinearGrid is a filter that converts an input vtkTable to a vtkRectilinearGrid.
 *
 * The three columns to extract the coordinates from are provided through the `SetXYZColumns`
 * methods.
 *
 * Each line of the input vtkTable contains a point data for an [i, j, k] point.
 * Some points may be missing in the input: they are mark as blank. (see
 * vtkRectilinearGrid::BlankPoint)
 *
 * This filter uses the vtkImplicitArray framework to reuse input data memory.
 *
 *
 * Lets have this table for a 2D example
 * ```
 * A, B, C
 * 0, 0, 10
 * 1, 1, 11
 * 0, 1, 12
 * 0, 2, 13
 * 1, 2, 14
 * ```
 * Using A as X and B as Y leads to this grid, with C value displayed at point position:
 * ```
 *    G   ─  ─  11───────14
 *              │        │
 *    |  Blank  │        │
 *              │        │
 *    10  ─  ─  12───────13
 * ```
 * Explanation:
 *
 * - A has values in (0, 1): X dimension is 2
 * - B has values in (0, 1, 2): Y dimensions is 3
 * - Looking for a (a, b) tuple in the rows give us a PointData for the (a, b) point
 * - The table does not contain a row where A=1 and B=0. So the point G is a blank point. Thus the
 * cell 0 (on the left) is blanked too.
 *
 * @see vtkTableToPoint, vtkTableToStructuredGrid
 */

#ifndef vtkTableToRectilinearGrid_h
#define vtkTableToRectilinearGrid_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"
#include "vtkSmartPointer.h" // for smart pointer

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkRectilinearGrid;
class vtkTable;

class VTKFILTERSGENERAL_EXPORT vtkTableToRectilinearGrid : public vtkRectilinearGridAlgorithm
{
public:
  static vtkTableToRectilinearGrid* New();
  vtkTypeMacro(vtkTableToRectilinearGrid, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the column to use as X coordinates.
   */
  void SetXColumn(const std::string& name);
  std::string GetXColumn();
  ///@}

  ///@{
  /**
   * Set/Get the name of the column to use as Y coordinates.
   */
  void SetYColumn(const std::string& name);
  std::string GetYColumn();
  ///@}

  ///@{
  /**
   * Set/Get the name of the column to use as Z coordinates.
   */
  void SetZColumn(const std::string& name);
  std::string GetZColumn();
  ///@}

  /**
   * Convenience method to set the three column names, in X, Y and Z order.
   */
  void SetXYZColumns(const std::string& xname, const std::string& yname, const std::string& zname);

protected:
  vtkTableToRectilinearGrid();
  ~vtkTableToRectilinearGrid() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Overriden to accept only vtkTable as input.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTableToRectilinearGrid(const vtkTableToRectilinearGrid&) = delete;
  void operator=(const vtkTableToRectilinearGrid&) = delete;

  /**
   * Set the three coordinates array on the grid.
   * If X, Y or Z column name was not set, fallback to `GetInputArrayToProcess`.
   */
  void SetCoordinateArrays(vtkInformationVector** inputInfo, vtkRectilinearGrid* output);

  /**
   * Fill coords array with unique, sorted values from the given column.
   * Return true on success, false on any error.
   */
  bool ComputeCoordinateArray(vtkDataArray* column, vtkDoubleArray* coords);

  /**
   * Transform each table column into a PointData array for the output grid.
   * Points that does not exist in the input will be masked, and their associated pointdata will
   * be the same as the first row. This should not be an issue as blanked points should be ignored
   * in further computation.
   * The cells containing a masked point are also masked.
   */
  void ColumnsToPointData(vtkTable* table, vtkRectilinearGrid* grid, vtkIdTypeArray* pointToRow);

  /**
   * Create a mapping from output point id to input row id.
   */
  vtkSmartPointer<vtkIdTypeArray> ComputePointToRowMap(
    vtkInformationVector** inputInfo, vtkTable* inputTable, vtkRectilinearGrid* grid);

  /**
   * Mark missing points input as blank. Map them to the first row.
   * Mark associated cell as blank too.
   * We cannot initialize the map to 0 because we should be able to detect
   * missing point id 0 when relevant.
   */
  void MarkBlanks(vtkRectilinearGrid* grid, vtkIdTypeArray* map);
};

VTK_ABI_NAMESPACE_END
#endif
