/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkAbstractRenderDevice_h
#define vtkAbstractRenderDevice_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"
#include <string> // For std::string

class vtkRecti;

class VTKRENDERINGCORE_EXPORT vtkAbstractRenderDevice : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractRenderDevice, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

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
   * @return True on success, false on failure.
   */
  virtual bool CreateNewWindow(const vtkRecti &geometry,
                               const std::string &name) = 0;

  /**
   * @brief Make the context current so that it can be used by OpenGL. This is
   * an expensive call, and so its use should be minimized to once per render
   * ideally.
   */
  virtual void MakeCurrent() = 0;

protected:
  vtkAbstractRenderDevice();
  ~vtkAbstractRenderDevice();

  int GLMajor;
  int GLMinor;

private:
  vtkAbstractRenderDevice(const vtkAbstractRenderDevice&);  // Not implemented.
  void operator=(const vtkAbstractRenderDevice&);  // Not implemented.
};

#endif
