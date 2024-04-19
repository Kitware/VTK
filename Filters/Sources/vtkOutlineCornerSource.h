// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOutlineCornerSource
 * @brief   create wireframe outline corners around bounding box
 *
 * vtkOutlineCornerSource creates wireframe outline corners around a user-specified
 * bounding box.
 */

#ifndef vtkOutlineCornerSource_h
#define vtkOutlineCornerSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkOutlineSource.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkOutlineCornerSource : public vtkOutlineSource
{
public:
  vtkTypeMacro(vtkOutlineCornerSource, vtkOutlineSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct outline corner source with default corner factor = 0.2
   */
  static vtkOutlineCornerSource* New();

  ///@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  vtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  vtkGetMacro(CornerFactor, double);
  ///@}

protected:
  vtkOutlineCornerSource();
  ~vtkOutlineCornerSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double CornerFactor;

private:
  vtkOutlineCornerSource(const vtkOutlineCornerSource&) = delete;
  void operator=(const vtkOutlineCornerSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
