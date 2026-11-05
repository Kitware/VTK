// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkmConvertToVTKArrays
 * @brief   Replace the vtkmDataArrays in a dataset with ordinary VTK arrays.
 *
 * Viskores-accelerated filters leave the arrays they compute on a GPU in a
 * vtkmDataArray, so the next Viskores filter can read them in place. Code
 * that is not Viskores-aware can read those through the vtkDataArray API, one
 * value at a time. This filter is for code that needs more than that -- a
 * raw host pointer, or a down-cast to a concrete array type -- and marks the
 * point in a pipeline where its data leave the device.
 *
 * The output is a shallow copy of the input in which every vtkmDataArray has
 * been replaced by a copy it owns (see vtkmDataArray::NewVTKArray()): point,
 * cell and field data, point coordinates, rectilinear coordinates, and the
 * cell arrays and cell types of polydata and unstructured grids. All other
 * arrays are shared with the input, and the input is not modified.
 */

#ifndef vtkmConvertToVTKArrays_h
#define vtkmConvertToVTKArrays_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmConvertToVTKArrays : public vtkDataSetAlgorithm
{
public:
  static vtkmConvertToVTKArrays* New();
  vtkTypeMacro(vtkmConvertToVTKArrays, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkmConvertToVTKArrays();
  ~vtkmConvertToVTKArrays() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmConvertToVTKArrays(const vtkmConvertToVTKArrays&) = delete;
  void operator=(const vtkmConvertToVTKArrays&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmConvertToVTKArrays_h
