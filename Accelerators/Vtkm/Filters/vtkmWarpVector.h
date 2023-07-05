// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmWarpVector
 * @brief   deform geometry with vector data
 *
 * vtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
 */

#ifndef vtkmWarpVector_h
#define vtkmWarpVector_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // required for correct export
#include "vtkWarpVector.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmWarpVector : public vtkWarpVector
{
public:
  vtkTypeMacro(vtkmWarpVector, vtkWarpVector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmWarpVector* New();

protected:
  vtkmWarpVector();
  ~vtkmWarpVector() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmWarpVector(const vtkmWarpVector&) = delete;
  void operator=(const vtkmWarpVector&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmWarpVector_h
