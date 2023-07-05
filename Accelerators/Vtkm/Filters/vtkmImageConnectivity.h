// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmImageConnectivity
 * @brief   Label regions inside an image by connectivity
 *
 * vtkmImageConnectivity will identify connected regions within an
 * image and label them.
 * The filter finds groups of points that have the same field value and are
 * connected together through their topology. Any point is considered to be
 * connected to its Moore neighborhood:
 * - 8 neighboring points for 2D
 * - 27 neighboring points for 3D
 *
 * The active field passed to the filter must be associated with the points.
 * The result of the filter is a point field of type vtkIdType.
 * Each entry in the point field will be a number that identifies to which
 * region it belongs. By default, this output point field is named “component”.
 *
 * @sa
 * vtkConnectivityFilter, vtkImageConnectivityFilter
 */

#ifndef vtkmImageConnectivity_h
#define vtkmImageConnectivity_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkImageAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmImageConnectivity : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkmImageConnectivity, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmImageConnectivity* New();

protected:
  vtkmImageConnectivity();
  ~vtkmImageConnectivity() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmImageConnectivity(const vtkmImageConnectivity&) = delete;
  void operator=(const vtkmImageConnectivity&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmImageConnectivity_h
