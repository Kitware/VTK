// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEdgePoints
 * @brief   generate points on isosurface
 *
 * vtkEdgePoints is a filter that takes as input any dataset and
 * generates for output a set of points that lie on an isosurface. The
 * points are created by interpolation along cells edges whose end-points are
 * below and above the contour value.
 * @warning
 * vtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
 * (see vtkDividingCubes). Points are generated only on the edges of cells,
 * not in the interior, and at lower density than dividing cubes. However, it
 * is more general than dividing cubes since it treats any type of dataset.
 */

#ifndef vtkEdgePoints_h
#define vtkEdgePoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMergePoints;

class VTKFILTERSGENERAL_EXPORT vtkEdgePoints : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEdgePoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with contour value of 0.0.
   */
  static vtkEdgePoints* New();

  ///@{
  /**
   * Set/get the contour value.
   */
  vtkSetMacro(Value, double);
  vtkGetMacro(Value, double);
  ///@}

protected:
  vtkEdgePoints();
  ~vtkEdgePoints() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  double Value;
  vtkMergePoints* Locator;

private:
  vtkEdgePoints(const vtkEdgePoints&) = delete;
  void operator=(const vtkEdgePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
