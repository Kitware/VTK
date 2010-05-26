/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKApplication.h"

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
#include "vtkTDxQtUnixDevices.h"
#include <QWidget>
#include <X11/Xlib.h> // Needed for X types used in the public interface
#endif

// ----------------------------------------------------------------------------
QVTKApplication::QVTKApplication(int &Argc, char **Argv)
  : QApplication(Argc,Argv)
{
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  this->Devices=new vtkTDxQtUnixDevices;
  QObject::connect(this->Devices,SIGNAL(CreateDevice(vtkTDxDevice *)),
                   this,SLOT(setDevice(vtkTDxDevice *)));
#endif
}

// ----------------------------------------------------------------------------
QVTKApplication::~QVTKApplication()
{
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  delete this->Devices;
#endif
}

// ----------------------------------------------------------------------------
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
bool QVTKApplication::x11EventFilter(XEvent *event)
{ 
  // the only lines required in this method
  this->Devices->ProcessEvent(
    static_cast<vtkTDxUnixDeviceXEvent *>(event));
  return false;
}
#endif

// ----------------------------------------------------------------------------
#ifdef VTK_USE_TDX
void QVTKApplication::setDevice(vtkTDxDevice *device)
{
#ifdef Q_WS_X11
  emit CreateDevice(device);
#else
  (void)device; // to avoid warnings.
#endif
}
#endif
