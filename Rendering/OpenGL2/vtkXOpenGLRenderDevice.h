/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkXOpenGLRenderDevice_h
#define vtkXOpenGLRenderDevice_h

#include "vtkAbstractRenderDevice.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <X11/Xutil.h> // Needed for X types used in the public interface

class vtkXInteractionDevice;

class VTKRENDERINGOPENGL2_EXPORT vtkXOpenGLRenderDevice
  : public vtkAbstractRenderDevice
{
public:
  vtkTypeMacro(vtkXOpenGLRenderDevice, vtkAbstractRenderDevice)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkXOpenGLRenderDevice* New();

  virtual bool CreateNewWindow(const vtkRecti &geometry, const std::string &name);

  virtual void MakeCurrent();

protected:
  vtkXOpenGLRenderDevice();
  ~vtkXOpenGLRenderDevice();

  Window ParentId;
  Window WindowId;
  Display *DisplayId;
  Colormap ColorMap;

  bool OwnDisplay; // Do we own the display?
  bool OwnWindow; // Do we own the window?
  bool OffScreenRendering; // Is this for off screen rendering?
  bool Mapped;

  double Borders;

private:
  vtkXOpenGLRenderDevice(const vtkXOpenGLRenderDevice&);  // Not implemented.
  void operator=(const vtkXOpenGLRenderDevice&);  // Not implemented.

  class Private;
  Private *Internal;

  friend class vtkXInteractionDevice;
};

#endif
