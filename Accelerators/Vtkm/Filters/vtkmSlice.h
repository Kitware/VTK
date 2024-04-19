// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmSlice
 * @brief   generate
 *
 * vtkmSlice is a filter that takes as input a volume (e.g., 3D
 * structured point set) and generates
 *
 */

#ifndef vtkmSlice_h
#define vtkmSlice_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkCutter.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmSlice : public vtkCutter
{
public:
  vtkTypeMacro(vtkmSlice, vtkCutter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmSlice* New();

protected:
  /// \brief Check if the input dataset is supported by this filter
  ///
  /// Certain input dataset types are not currently supported by vtkm.
  /// This information is internally used to determine if this filter should fall back to
  /// Superclass implementation.
  bool CanProcessInput(vtkDataSet* input);

  vtkmSlice();
  ~vtkmSlice() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmSlice(const vtkmSlice&) = delete;
  void operator=(const vtkmSlice&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmSlice_h
