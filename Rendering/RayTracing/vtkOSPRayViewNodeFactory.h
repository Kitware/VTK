// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayViewNodeFactory
 * @brief   matches vtk rendering classes to
 * specific ospray ViewNode classes
 *
 * Ensures that vtkOSPRayPass makes ospray specific translator instances
 * for every VTK rendering pipeline class instance it encounters.
 */

#ifndef vtkOSPRayViewNodeFactory_h
#define vtkOSPRayViewNodeFactory_h

#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkViewNodeFactory.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayViewNodeFactory : public vtkViewNodeFactory
{
public:
  static vtkOSPRayViewNodeFactory* New();
  vtkTypeMacro(vtkOSPRayViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkOSPRayViewNodeFactory();
  ~vtkOSPRayViewNodeFactory() override;

private:
  vtkOSPRayViewNodeFactory(const vtkOSPRayViewNodeFactory&) = delete;
  void operator=(const vtkOSPRayViewNodeFactory&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
