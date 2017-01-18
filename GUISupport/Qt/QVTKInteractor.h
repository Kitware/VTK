/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

// .NAME QVTKInteractor - Handle Qt events.
// .SECTION Description
// QVTKInteractor handles relaying Qt events to VTK.

#ifndef Q_VTK_INTERACTOR_H
#define Q_VTK_INTERACTOR_H

#include "vtkGUISupportQtModule.h" // For export macro
#include "QVTKWin32Header.h"
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>
#include <QtCore/QObject>

#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
class vtkTDxWinDevice;
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
class vtkTDxMacDevice;
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
class vtkTDxDevice;
class vtkTDxUnixDevice;
#endif


class QVTKInteractorInternal;

// .NAME QVTKInteractor - An interactor for the QVTKWidget.
// .SECTION Description
// QVTKInteractor is an interactor for a QVTKWiget.

class VTKGUISUPPORTQT_EXPORT QVTKInteractor : public vtkRenderWindowInteractor
{
public:
  static QVTKInteractor* New();
  vtkTypeMacro(QVTKInteractor,vtkRenderWindowInteractor);

  // Description:
  // Enum for additional event types supported.
  // These events can be picked up by command observers on the interactor
  enum vtkCustomEvents
  {
    ContextMenuEvent = vtkCommand::UserEvent + 100,
    DragEnterEvent,
    DragMoveEvent,
    DragLeaveEvent,
    DropEvent
  };


  // Description:
  // Overloaded terminiate app, which does nothing in Qt.
  // Use qApp->exit() instead.
  void TerminateApp() VTK_OVERRIDE;

  // Description:
  // Overloaded start method does nothing.
  // Use qApp->exec() instead.
  void Start() VTK_OVERRIDE;
  void Initialize() VTK_OVERRIDE;

  // Description:
  // Start listening events on 3DConnexion device.
  virtual void StartListening();

  // Description:
  // Stop listening events on 3DConnexion device.
  virtual void StopListening();

  // timer event slot
  virtual void TimerEvent(int timerId);

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  virtual vtkTDxUnixDevice *GetDevice();
  virtual void SetDevice(vtkTDxDevice *device);
#endif

protected:
  // constructor
  QVTKInteractor();
  // destructor
  ~QVTKInteractor() VTK_OVERRIDE;

  // create a Qt Timer
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) VTK_OVERRIDE;
  // destroy a Qt Timer
  int InternalDestroyTimer(int platformTimerId) VTK_OVERRIDE;
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  vtkTDxWinDevice *Device;
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
  vtkTDxMacDevice *Device;
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  vtkTDxUnixDevice *Device;
#endif

private:

  QVTKInteractorInternal* Internal;

  // unimplemented copy
  QVTKInteractor(const QVTKInteractor&);
  // unimplemented operator=
  void operator=(const QVTKInteractor&);

};

#endif
