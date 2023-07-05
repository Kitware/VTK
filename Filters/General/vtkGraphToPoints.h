// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
/**
 * @class   vtkGraphToPoints
 * @brief   convert a vtkGraph a set of points.
 *
 *
 * Converts a vtkGraph to a vtkPolyData containing a set of points.
 * This assumes that the points
 * of the graph have already been filled (perhaps by vtkGraphLayout).
 * The vertex data is passed along to the point data.
 */

#ifndef vtkGraphToPoints_h
#define vtkGraphToPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkGraphToPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphToPoints* New();
  vtkTypeMacro(vtkGraphToPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGraphToPoints();
  ~vtkGraphToPoints() override = default;

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGraphToPoints(const vtkGraphToPoints&) = delete;
  void operator=(const vtkGraphToPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
