// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkTubeBender
 * @brief Rounds corners on lines for better tubes
 *
 * vtkTubeBender is designed to generate better tube paths for vtkTubeFilter.
 *
 * For points with very sharp inflection point angles, the radius used to determine
 * where surface points are placed becomes more parallel, instead of perpendicular,
 * to the tube path. This causes the tube to become more oblong, as the
 * cross-sectional height and width become increasingly disparate.
 *
 * This filter inserts new points along the tube path near acute angles to reduce
 * the amount the point normals will change from point to point. This reduces the
 * cross-sectional height and width variations from over 95% to less than 30%.
 * This gives an impression of a constant diameter tube with nice acute angle bends
 * without adding too many new points (and therefore faces).
 *
 *
 * @warning
 * Any vtkTubeFilters which use the output of this filter should set UseDefaultNormalOff.
 *
 * If the path has a relatively large radius and several acute angles in sequence,
 * the default normal generation can cause tube segments to have a torsional rotation,
 * along the tube's length which renders as an hourglass instead of a tube. Using
 * alternate normals correctly renders these hourglass segments as tubes. This problem
 * may only appear as the radius increases for a given path.
 *
 * @sa
 * vtkTubeFilter
 */

#ifndef vtkTubeBender_h
#define vtkTubeBender_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkTubeBender : public vtkPolyDataAlgorithm
{
public:
  static vtkTubeBender* New();
  // Generating VTK hierarchical class relationship
  vtkTypeMacro(vtkTubeBender, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the minimum tube radius (minimum because the tube radius may vary).
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

protected:
  vtkTubeBender();
  ~vtkTubeBender() override;

  double Radius;

public:
  vtkTubeBender(const vtkTubeBender&) = delete;  // Not implemented.
  void operator=(const vtkTubeBender&) = delete; // Not implemented.

protected:
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
};

VTK_ABI_NAMESPACE_END
#endif // vtkTubeBender_h
