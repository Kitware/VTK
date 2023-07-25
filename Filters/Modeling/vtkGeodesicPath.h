// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeodesicPath
 * @brief   Abstract base for classes that generate a geodesic path
 *
 * Serves as a base class for algorithms that trace a geodesic path on a
 * polygonal dataset.
 */

#ifndef vtkGeodesicPath_h
#define vtkGeodesicPath_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTKFILTERSMODELING_EXPORT vtkGeodesicPath : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for printing and determining type information.
   */
  vtkTypeMacro(vtkGeodesicPath, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkGeodesicPath();
  ~vtkGeodesicPath() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGeodesicPath(const vtkGeodesicPath&) = delete;
  void operator=(const vtkGeodesicPath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
