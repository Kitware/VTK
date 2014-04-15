/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXInteractionDevice.h"

#include "vtkObjectFactory.h"
#include "vtkXOpenGLRenderDevice.h"
#include "vtkRenderWidget.h"

#include <X11/Xlib.h>

#include <cassert>

vtkStandardNewMacro(vtkXInteractionDevice)

vtkXInteractionDevice::vtkXInteractionDevice()
  : DisplayId(NULL), ExitEventLoop(false)
{
}

vtkXInteractionDevice::~vtkXInteractionDevice()
{
}

void vtkXInteractionDevice::Initialize()
{
  // This is where any initialization tasks for our interaction device go.
  assert(this->RenderDevice);
  vtkXOpenGLRenderDevice *device
      = vtkXOpenGLRenderDevice::SafeDownCast(this->RenderDevice);
  assert(device);

  this->DisplayId = device->DisplayId;
  XSelectInput(device->DisplayId, device->WindowId,
               ExposureMask | KeyPressMask);
}

void vtkXInteractionDevice::Start()
{
  // Start the main event loop.
  assert(this->DisplayId);
  this->ExitEventLoop = false;
  XEvent event;
  for (;;)
    {
    XNextEvent(this->DisplayId, &event);
    cout << "New event..." << endl;
    this->ProcessEvent(event);
    if (this->ExitEventLoop)
      {
      break;
      }
  }
}

void vtkXInteractionDevice::ProcessEvents()
{
  assert(this->RenderDevice);
  XEvent event;
  XNextEvent(this->DisplayId, &event);
  this->ProcessEvent(event);
}

inline void vtkXInteractionDevice::ProcessEvent(XEvent &event)
{
  switch (event.type)
    {
    case MapNotify:
      break;
    case Expose:
      this->RenderWidget->Render();
      break;
    case KeyPress:
      this->ExitEventLoop = true;
      break;
    default:
      return;
    }
}

void vtkXInteractionDevice::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
