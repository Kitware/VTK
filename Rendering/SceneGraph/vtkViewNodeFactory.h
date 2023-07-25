// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkViewNodeFactory
 * @brief   factory that chooses vtkViewNodes to create
 *
 * Class tells VTK which specific vtkViewNode subclass to make when it is
 * asked to make a vtkViewNode for a particular renderable. modules for
 * different rendering backends are expected to use this to customize the
 * set of instances for their own purposes
 */

#ifndef vtkViewNodeFactory_h
#define vtkViewNodeFactory_h

#include "vtkObject.h"
#include "vtkRenderingSceneGraphModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkViewNode;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkViewNodeFactory : public vtkObject
{
public:
  static vtkViewNodeFactory* New();
  vtkTypeMacro(vtkViewNodeFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Give a function pointer to a class that will manufacture a
   * vtkViewNode when given a class name string.
   */
  void RegisterOverride(const char* name, vtkViewNode* (*func)());

  /**
   * Creates and returns a vtkViewNode for the provided renderable.
   */
  vtkViewNode* CreateNode(vtkObject*);

protected:
  vtkViewNodeFactory();
  ~vtkViewNodeFactory() override;

private:
  vtkViewNodeFactory(const vtkViewNodeFactory&) = delete;
  void operator=(const vtkViewNodeFactory&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
