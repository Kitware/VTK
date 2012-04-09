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
// .NAME QVTKApplication - A superclass for QApplication using VTK.
// .SECTION Description
// This is a superclass for QApplication using VTK. It essentially redefined
// x11EventFilter() in order to catch X11 ClientMessage coming from the
// 3DConnexion driver.
//
// You don't have to inherit from QVTKApplication to be able to use VTK:
// you can reimplement QVTKApplication(), ~QVTKApplication(), x11EventFilter(),
// setDevice(), CreateDevice() in your own subclass of QApplication.
// It you don't, VTK will work but without the 3Dconnexion device under X11.
// In this case, QVTKApplication provides a model of implementation.

// .SECTION See Also
// vtkTDxQtUnixDevices QVTKWidget

#ifndef __QVTKApplication_h
#define __QVTKApplication_h


#include "vtkGUISupportQtModule.h" // For export macro
#include "QVTKWin32Header.h" // for VTKGUISUPPORTQT_EXPORT
#include "vtkTDxConfigure.h" // defines VTK_USE_TDX

#include <QApplication>

#ifdef VTK_USE_TDX
class vtkTDxDevice;
 #ifdef Q_WS_X11
class vtkTDxQtUnixDevices;
 #endif
#endif

class VTKGUISUPPORTQT_EXPORT QVTKApplication : public QApplication
{
   Q_OBJECT
public:
   // Description:
   // Constructor.
   QVTKApplication(int &Argc, char **Argv);

  // Description:
  // Destructor.
  ~QVTKApplication();

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  // Description:
  // Intercept X11 events.
  // Redefined from QApplication.
  virtual bool x11EventFilter(XEvent *event);
#endif

#ifdef VTK_USE_TDX
public Q_SLOTS:
// Description:
// Slot to receive signal CreateDevice coming from vtkTDxQtUnixDevices.
// It re-emit signal CreateDevice (to QVTKWidget slots)
// No-op if not X11 (ie Q_WS_X11 is not defined).
void setDevice(vtkTDxDevice *device);

Q_SIGNALS:
// Description:
// Signal for VTKWidget slots.
   void CreateDevice(vtkTDxDevice *device);
#endif

protected:
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  vtkTDxQtUnixDevices *Devices;
#endif
};

#endif
