// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericOutlineFilter
 * @brief   create wireframe outline for arbitrary
 * generic data set
 *
 *
 * vtkGenericOutlineFilter is a filter that generates a wireframe outline of
 * any generic data set. The outline consists of the twelve edges of the
 * generic dataset bounding box.
 *
 * @sa
 * vtkGenericDataSet
 */

#ifndef vtkGenericOutlineFilter_h
#define vtkGenericOutlineFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOutlineSource;

class VTKFILTERSGENERIC_EXPORT vtkGenericOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGenericOutlineFilter* New();
  vtkTypeMacro(vtkGenericOutlineFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGenericOutlineFilter();
  ~vtkGenericOutlineFilter() override;

  vtkOutlineSource* OutlineSource;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkGenericOutlineFilter(const vtkGenericOutlineFilter&) = delete;
  void operator=(const vtkGenericOutlineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
