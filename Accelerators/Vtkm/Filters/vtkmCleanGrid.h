// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmCleanGrid
 * @brief   removes redundant or unused cells and/or points
 *
 * vtkmCleanGrid is a filter that takes vtkDataSet data as input and
 * generates vtkUnstructuredGrid as output. vtkmCleanGrid will convert all cells
 * to an explicit representation, and if enabled, will remove unused points.
 *
 */

#ifndef vtkmCleanGrid_h
#define vtkmCleanGrid_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkmAlgorithm.h"           // For vtkmAlgorithm
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#ifndef __VTK_WRAP__
#define vtkUnstructuredGridAlgorithm vtkmAlgorithm<vtkUnstructuredGridAlgorithm>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkUnstructuredGrid;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmCleanGrid : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkmCleanGrid, vtkUnstructuredGridAlgorithm);
#ifndef __VTK_WRAP__
#undef vtkUnstructuredGridAlgorithm
#endif
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmCleanGrid* New();

  ///@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. On by default.
   */
  vtkSetMacro(CompactPoints, bool);
  vtkGetMacro(CompactPoints, bool);
  vtkBooleanMacro(CompactPoints, bool);
  ///@}

  ///@{
  /**
   * Get/Set if the points that are coincident should be merged. The distance
   * two points can be to considered coincident is set with the tolerance flags.
   * This is on by default.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  ///@}

  ///@{
  /**
   * Get/Set the tolerance used when determining whether two points are
   * considered coincident. Because floating point parameters have limited
   * precision, point coordinates that are essentially the same might not be
   * bit-wise exactly the same. Thus, the `CleanGrid` filter has the ability to
   * find and merge points that are close but perhaps not exact. If the
   * ToleranceIsAbsolute flag is false (the default), then this tolerance is
   * scaled by the diagonal of the points.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get/Set if the `Tolerance` should be considered relative to the bound of
   * the data or absolue. When `ToleranceIsAbsolute` is off then the tolerance
   * is scaled by the diagonal of the bounds of the dataset. If on, then the
   * tolerance is taken as the actual distance to use. This is off by sefault
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  ///@}

  ///@{
  /**
   * Get/Set if degenerate cells are removed. When on, `CleanGrid` will look
   * for repeated points in cells and, if the repeated points cause the cell to drop
   * dimensionality, the cell is removed. This is particularly useful when point merging
   * is on as this operation can create degenerate cells. On by default.
   */
  vtkSetMacro(RemoveDegenerateCells, bool);
  vtkGetMacro(RemoveDegenerateCells, bool);
  vtkBooleanMacro(RemoveDegenerateCells, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether to use faster but less precise point merging. When
   * FastMerge is true (the default), some corners are cut when computing
   * coincident points. The point merge will go faster but the tolerance will
   * not be strictly followed.
   */
  vtkSetMacro(FastMerge, bool);
  vtkGetMacro(FastMerge, bool);
  vtkBooleanMacro(FastMerge, bool);
  ///@}

protected:
  vtkmCleanGrid();
  ~vtkmCleanGrid() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool CompactPoints = true;
  bool MergePoints = true;
  double Tolerance = 1.0e-6;
  bool ToleranceIsAbsolute = false;
  bool RemoveDegenerateCells = true;
  bool FastMerge = true;

private:
  vtkmCleanGrid(const vtkmCleanGrid&) = delete;
  void operator=(const vtkmCleanGrid&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmCleanGrid_h
