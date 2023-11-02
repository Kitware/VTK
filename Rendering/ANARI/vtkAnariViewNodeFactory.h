// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariViewNodeFactory
 * @brief   matches vtk rendering classes to specific ANARI ViewNode classes
 *
 * Ensures that vtkAnariPass makes ANARI specific translator instances
 * for every VTK rendering pipeline class instance it encounters.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariViewNodeFactory_h
#define vtkAnariViewNodeFactory_h

#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkViewNodeFactory.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT vtkAnariViewNodeFactory : public vtkViewNodeFactory
{
public:
  static vtkAnariViewNodeFactory* New();
  vtkTypeMacro(vtkAnariViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkAnariViewNodeFactory();
  ~vtkAnariViewNodeFactory() = default;

private:
  vtkAnariViewNodeFactory(const vtkAnariViewNodeFactory&) = delete;
  void operator=(const vtkAnariViewNodeFactory&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
