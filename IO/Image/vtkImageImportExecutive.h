// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageImportExecutive
 *
 * vtkImageImportExecutive
 */

#ifndef vtkImageImportExecutive_h
#define vtkImageImportExecutive_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkImageImportExecutive : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkImageImportExecutive* New();
  vtkTypeMacro(vtkImageImportExecutive, vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override to implement some requests with callbacks.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

protected:
  vtkImageImportExecutive() = default;
  ~vtkImageImportExecutive() override = default;

private:
  vtkImageImportExecutive(const vtkImageImportExecutive&) = delete;
  void operator=(const vtkImageImportExecutive&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
