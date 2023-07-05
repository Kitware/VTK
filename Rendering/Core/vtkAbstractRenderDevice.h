// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkAbstractRenderDevice_h
#define vtkAbstractRenderDevice_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include <string>                   // For std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkRecti;

class VTKRENDERINGCORE_EXPORT vtkAbstractRenderDevice : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractRenderDevice, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Make a new device, this class is abstract and one of its derived
   * forms will be returned, or NULL if no override has been provided.
   * @return A derived render device, or NULL if no suitable override is set.
   */
  static vtkAbstractRenderDevice* New();

  /**
   * @brief Set the context that should be requested (must be set before the
   * widget is rendered for the first time.
   * @param major Major GL version, default is 2.
   * @param minor Minor GL version, default is 1.
   */
  void SetRequestedGLVersion(int major, int minor);

  /**
   * @brief Create a window with the desired geometry.
   * @param geometry The geometry in screen coordinates for the window.
   * @param name The name of the window.
   * @return True on success, false on failure.
   */
  virtual bool CreateNewWindow(const vtkRecti& geometry, const std::string& name) = 0;

  /**
   * @brief Make the context current so that it can be used by OpenGL. This is
   * an expensive call, and so its use should be minimized to once per render
   * ideally.
   */
  virtual void MakeCurrent() = 0;

protected:
  vtkAbstractRenderDevice();
  ~vtkAbstractRenderDevice() override;

  int GLMajor;
  int GLMinor;

private:
  vtkAbstractRenderDevice(const vtkAbstractRenderDevice&) = delete;
  void operator=(const vtkAbstractRenderDevice&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
