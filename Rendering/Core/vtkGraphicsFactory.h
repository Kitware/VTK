// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphicsFactory
 *
 */

#ifndef vtkGraphicsFactory_h
#define vtkGraphicsFactory_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkGraphicsFactory : public vtkObject
{
public:
  static vtkGraphicsFactory* New();
  vtkTypeMacro(vtkGraphicsFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create and return an instance of the named vtk object.
   * This method first checks the vtkObjectFactory to support
   * dynamic loading.
   */
  VTK_NEWINSTANCE
  static vtkObject* CreateInstance(const char* vtkclassname);

  /**
   * What rendering library has the user requested
   */
  static const char* GetRenderLibrary();

  ///@{
  /**
   * This option enables the creation of Mesa classes
   * instead of the OpenGL classes when using mangled Mesa.
   */
  static void SetUseMesaClasses(int use);
  static int GetUseMesaClasses();
  ///@}

  ///@{
  /**
   * This option enables the off-screen only mode. In this mode no X calls will
   * be made even when interactor is used.
   */
  static void SetOffScreenOnlyMode(int use);
  static int GetOffScreenOnlyMode();
  ///@}

protected:
  vtkGraphicsFactory() = default;

  static int UseMesaClasses;
  static int OffScreenOnlyMode;

private:
  vtkGraphicsFactory(const vtkGraphicsFactory&) = delete;
  void operator=(const vtkGraphicsFactory&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
