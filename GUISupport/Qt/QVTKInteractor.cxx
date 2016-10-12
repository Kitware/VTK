/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#include "QVTKInteractor.h"
#include "QVTKInteractorInternal.h"

#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
# include "vtkTDxWinDevice.h"
#endif

#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
# include "vtkTDxMacDevice.h"
#endif

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
# include "vtkTDxUnixDevice.h"
#endif

#include <QEvent>
#include <QSignalMapper>
#include <QTimer>
#include <QResizeEvent>

#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkRenderWindow.h"

QVTKInteractorInternal::QVTKInteractorInternal(QVTKInteractor* p)
  : Parent(p)
{
  this->SignalMapper = new QSignalMapper(this);
  QObject::connect(this->SignalMapper, SIGNAL(mapped(int)), this, SLOT(TimerEvent(int)) );
}

QVTKInteractorInternal::~QVTKInteractorInternal()
{
}

void QVTKInteractorInternal::TimerEvent(int id)
{
  Parent->TimerEvent(id);
}

/*! allocation method for Qt/VTK interactor
 */
vtkStandardNewMacro(QVTKInteractor);

/*! constructor for Qt/VTK interactor
 */
QVTKInteractor::QVTKInteractor()
{
  this->Internal = new QVTKInteractorInternal(this);

#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  this->Device=vtkTDxWinDevice::New();
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
  this->Device=vtkTDxMacDevice::New();
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  this->Device=0;
#endif
}

void QVTKInteractor::Initialize()
{
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  if(this->UseTDx)
  {
    // this is QWidget::winId();
    HWND hWnd=static_cast<HWND>(this->GetRenderWindow()->GetGenericWindowId());
    if(!this->Device->GetInitialized())
    {
      this->Device->SetInteractor(this);
      this->Device->SetWindowHandle(hWnd);
      this->Device->Initialize();
    }
  }
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
  if(this->UseTDx)
  {
    if(!this->Device->GetInitialized())
    {
      this->Device->SetInteractor(this);
      // Do not initialize the device here.
    }
  }
#endif
  this->Initialized = 1;
  this->Enable();
}

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
// ----------------------------------------------------------------------------
vtkTDxUnixDevice *QVTKInteractor::GetDevice()
{
  return this->Device;
}

// ----------------------------------------------------------------------------
void QVTKInteractor::SetDevice(vtkTDxDevice *device)
{
  if(this->Device!=device)
  {
    this->Device=static_cast<vtkTDxUnixDevice *>(device);
  }
}
#endif


/*! start method for interactor
 */
void QVTKInteractor::Start()
{
  vtkErrorMacro(<<"QVTKInteractor cannot control the event loop.");
}

/*! terminate the application
 */
void QVTKInteractor::TerminateApp()
{
  // we are in a GUI so let's terminate the GUI the normal way
  //qApp->exit();
}

// ----------------------------------------------------------------------------
void QVTKInteractor::StartListening()
{
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  if(this->Device->GetInitialized() && !this->Device->GetIsListening())
  {
    this->Device->StartListening();
  }
#endif
#if defined(VTK_USE_TDX) &&  defined(Q_OS_MAC)
  if(this->UseTDx && !this->Device->GetInitialized())
  {
    this->Device->Initialize();
  }
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  if(this->UseTDx && this->Device!=0)
  {
    this->Device->SetInteractor(this);
  }
#endif
}

// ----------------------------------------------------------------------------
void QVTKInteractor::StopListening()
{
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  if(this->Device->GetInitialized() && this->Device->GetIsListening())
  {
    this->Device->StopListening();
  }
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
  if(this->UseTDx && this->Device->GetInitialized())
  {
    this->Device->Close();
  }
#endif
#if defined(VTK_USE_TDX) &&  defined(Q_WS_X11)
  if(this->UseTDx && this->Device!=0)
  {
    // this assumes that a outfocus event is emitted prior
    // a infocus event on another widget.
    this->Device->SetInteractor(0);
  }
#endif
}


/*! handle timer event
 */
void QVTKInteractor::TimerEvent(int timerId)
{
  if ( !this->GetEnabled() )
  {
    return;
  }
  this->InvokeEvent(vtkCommand::TimerEvent, (void*)&timerId);

  if(this->IsOneShotTimer(timerId))
  {
    this->DestroyTimer(timerId);  // 'cause our Qt timers are always repeating
  }
}

/*! constructor
 */
QVTKInteractor::~QVTKInteractor()
{
  delete this->Internal;
#if defined(VTK_USE_TDX) && defined(Q_OS_WIN)
  this->Device->Delete();
#endif
#if defined(VTK_USE_TDX) && defined(Q_OS_MAC)
  this->Device->Delete();
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
  this->Device=0;
#endif
}

/*! create Qt timer with an interval of 10 msec.
 */
int QVTKInteractor::InternalCreateTimer(int timerId, int vtkNotUsed(timerType), unsigned long duration)
{
  QTimer* timer = new QTimer(this->Internal);
  timer->start(duration);
  this->Internal->SignalMapper->setMapping(timer, timerId);
  QObject::connect(timer, SIGNAL(timeout()), this->Internal->SignalMapper, SLOT(map()));
  int platformTimerId = timer->timerId();
  this->Internal->Timers.insert(QVTKInteractorInternal::TimerMap::value_type(platformTimerId, timer));
  return platformTimerId;
}

/*! destroy timer
 */
int QVTKInteractor::InternalDestroyTimer(int platformTimerId)
{
  QVTKInteractorInternal::TimerMap::iterator iter = this->Internal->Timers.find(platformTimerId);
  if(iter != this->Internal->Timers.end())
  {
    iter->second->stop();
    iter->second->deleteLater();
    this->Internal->Timers.erase(iter);
    return 1;
  }
  return 0;
}
