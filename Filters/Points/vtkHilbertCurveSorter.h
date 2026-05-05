// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkHilbertCurveSorter
 * @brief   Sort point coordinates using Hilbert-curve
 *
 * vtkHilbertCurveSorter provides reorder points using the hilbert curve.
 * Reordering points this way improves spatial locality: points that are
 * close in 3D space receive nearby indices, which in turn improves cache
 * performance for algorithms that access point coordinates by index
 * (e.g., Delaunay triangulation, Voronoi tessellation, spatial locators).
 *
 * The implementation uses the algorithm described by Skilling (AIP Conf.
 * Proc. 707, 381, 2004) to convert axis-aligned coordinates into a
 * transposed Hilbert index.
 */

#ifndef vtkHilbertCurveSorter_h
#define vtkHilbertCurveSorter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSPOINTS_EXPORT vtkHilbertCurveSorter : public vtkPassInputTypeAlgorithm
{
public:
  ///@{
  /**
   * Standard VTK methods for instantiating, managing type, and printing
   * information about this class.
   */
  static vtkHilbertCurveSorter* New();
  vtkTypeMacro(vtkHilbertCurveSorter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Return the permutation of points due to hilbert curve sorting.
   */
  vtkGetObjectMacro(Permutation, vtkIdList);

  ///@{
  /**
   * If ComputePermutationOnly is true, the filter will compute the Hilbert permutation
   * but will not reorder the points in the output.
   *
   * Default is off.
   */
  vtkSetMacro(ComputePermutationOnly, bool);
  vtkBooleanMacro(ComputePermutationOnly, bool);
  vtkGetMacro(ComputePermutationOnly, bool);
  ///@}

protected:
  vtkHilbertCurveSorter() = default;
  ~vtkHilbertCurveSorter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;

private:
  vtkHilbertCurveSorter(const vtkHilbertCurveSorter&) = delete;
  void operator=(const vtkHilbertCurveSorter&) = delete;

  vtkNew<vtkIdList> Permutation;
  bool ComputePermutationOnly = false;
};

VTK_ABI_NAMESPACE_END
#endif
