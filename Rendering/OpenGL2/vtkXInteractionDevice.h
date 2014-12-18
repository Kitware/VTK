/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkXInteractionDevice_h
#define vtkXInteractionDevice_h

#include "vtkAbstractInteractionDevice.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <X11/Xlib.h> // Needed for X types used in the public interface

class VTKRENDERINGOPENGL2_EXPORT vtkXInteractionDevice
    : public vtkAbstractInteractionDevice
{
public:
  vtkTypeMacro(vtkXInteractionDevice, vtkAbstractInteractionDevice)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkXInteractionDevice* New();

  virtual void Initialize();

  virtual void Start();

  virtual void ProcessEvents();

protected:
  vtkXInteractionDevice();
  ~vtkXInteractionDevice();

  Display *DisplayId;
  bool ExitEventLoop;

private:
  vtkXInteractionDevice(const vtkXInteractionDevice&);  // Not implemented.
  void operator=(const vtkXInteractionDevice&);  // Not implemented.

  void ProcessEvent(XEvent &event);
};

#endif
