// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointConnectivityFilter
 * @brief   output a scalar field indicating point connectivity
 *
 *
 * vtkPointConnectivityFilter is a filter the produces a point scalar field
 * that characerizes the connectivity of the point. What is meant by
 * connectivity is the number of cells that use each point. The output
 * scalar array is represented by a 16-bit integral value. A value of zero
 * means that no cells use a particular point.
 */

#ifndef vtkPointConnectivityFilter_h
#define vtkPointConnectivityFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkPointConnectivityFilter : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information and
   * printing.
   */
  static vtkPointConnectivityFilter* New();
  vtkTypeMacro(vtkPointConnectivityFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkPointConnectivityFilter();
  ~vtkPointConnectivityFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointConnectivityFilter(const vtkPointConnectivityFilter&) = delete;
  void operator=(const vtkPointConnectivityFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
