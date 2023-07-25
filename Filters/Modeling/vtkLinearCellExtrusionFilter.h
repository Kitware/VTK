// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLinearCellExtrusionFilter
 * @brief   extrude polygonal data to create 3D cells from 2D cells
 *
 * vtkLinearCellExtrusionFilter is a modeling filter. It takes polygonal data as
 * input and generates an unstructured grid data on output. The input dataset is swept
 * according to the input cell data array value along the cell normal and creates
 * new 3D primitives.
 * Triangles will become Wedges, Quads will become Hexahedrons,
 * and Polygons will become Polyhedrons.
 * This filter currently takes into account only polys and discard vertices, lines and strips.
 * Unlike the vtkLinearExtrusionFilter, this filter is designed to extrude each cell independently
 * using its normal and its scalar value.
 *
 * @sa
 * vtkLinearExtrusionFilter
 */

#ifndef vtkLinearCellExtrusionFilter_h
#define vtkLinearCellExtrusionFilter_h

#include "vtkFiltersModelingModule.h"   // For export macro
#include "vtkIncrementalPointLocator.h" // For vtkIncrementalPointLocator
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // For smart pointer

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSMODELING_EXPORT vtkLinearCellExtrusionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkLinearCellExtrusionFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLinearCellExtrusionFilter* New();

  ///@{
  /**
   * Specify the scale factor applied on the cell value during extrusion.
   * Default is 1.0
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Specify if the algorithm should use the specified vector instead of cell normals.
   * Default is false
   */
  vtkSetMacro(UseUserVector, bool);
  vtkGetMacro(UseUserVector, bool);
  vtkBooleanMacro(UseUserVector, bool);
  ///@}

  ///@{
  /**
   * Specify the scale factor applied on the cell value during extrusion.
   */
  vtkSetVector3Macro(UserVector, double);
  vtkGetVector3Macro(UserVector, double);
  ///@}

  ///@{
  /**
   * Specify if the algorithm should merge duplicate points.
   * Default is false
   */
  vtkSetMacro(MergeDuplicatePoints, bool);
  vtkGetMacro(MergeDuplicatePoints, bool);
  vtkBooleanMacro(MergeDuplicatePoints, bool);
  ///@}

  ///@{
  /**
   * Specify a spatial locator for merging points.
   * By default, an instance of vtkMergePoints is used.
   */
  vtkGetSmartPointerMacro(Locator, vtkIncrementalPointLocator);
  vtkSetSmartPointerMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  vtkLinearCellExtrusionFilter();
  ~vtkLinearCellExtrusionFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  double ScaleFactor = 1.0;
  double UserVector[3] = { 0.0, 0.0, 1.0 };
  bool UseUserVector = false;
  bool MergeDuplicatePoints = false;
  vtkSmartPointer<vtkIncrementalPointLocator> Locator;

private:
  vtkLinearCellExtrusionFilter(const vtkLinearCellExtrusionFilter&) = delete;
  void operator=(const vtkLinearCellExtrusionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
