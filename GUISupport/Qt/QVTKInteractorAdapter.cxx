/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKInteractorAdapter.cxx

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

#include "QVTKInteractorAdapter.h"
#include "QVTKInteractor.h"

#include <QEvent>
#include <QSignalMapper>
#include <QTimer>
#include <QResizeEvent>

#include "vtkCommand.h"

// function to get VTK keysyms from ascii characters
static const char* ascii_to_key_sym(int);
// function to get VTK keysyms from Qt keys
static const char* qt_key_to_key_sym(Qt::Key, Qt::KeyboardModifiers modifiers);

QVTKInteractorAdapter::QVTKInteractorAdapter(QObject* parentObject)
  : QObject(parentObject), AccumulatedDelta(0), DevicePixelRatio(1)
{
}

QVTKInteractorAdapter::~QVTKInteractorAdapter()
{
}

void QVTKInteractorAdapter::SetDevicePixelRatio(int ratio, vtkRenderWindowInteractor* iren)
{
  if (ratio != DevicePixelRatio)
  {
    if (iren)
    {
      int tmp[2];
      iren->GetSize(tmp);
      if (ratio == 1)
      {
        iren->SetSize(tmp[0] / 2, tmp[1] / 2);
      }
      else if (ratio == 2)
      {
        iren->SetSize(tmp[0] * 2, tmp[1] * 2);
      }
    }
    this->DevicePixelRatio = ratio;
  }
}

bool QVTKInteractorAdapter::ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren)
{
  if(iren == NULL || e == NULL)
    return false;

  const QEvent::Type t = e->type();

  if(t == QEvent::Resize)
  {
    QResizeEvent* e2 = static_cast<QResizeEvent*>(e);
    QSize size = e2->size();
    iren->SetSize(size.width() * this->DevicePixelRatio,
                  size.height() * this->DevicePixelRatio);
    iren->InvokeEvent(vtkCommand::ConfigureEvent, e2);
    return true;
  }

  if(t == QEvent::FocusIn)
  {
    // For 3DConnexion devices:
    QVTKInteractor* qiren = QVTKInteractor::SafeDownCast(iren);
    if(qiren)
    {
      qiren->StartListening();
    }
    return true;
  }

  if(t == QEvent::FocusOut)
  {
    // For 3DConnexion devices:
    QVTKInteractor* qiren = QVTKInteractor::SafeDownCast(iren);
    if(qiren)
    {
      qiren->StopListening();
    }
    return true;
  }

  // the following events only happen if the interactor is enabled
  if(!iren->GetEnabled())
    return false;

  if(t == QEvent::MouseButtonPress ||
     t == QEvent::MouseButtonRelease ||
     t == QEvent::MouseButtonDblClick ||
     t == QEvent::MouseMove)
  {
    QMouseEvent* e2 = static_cast<QMouseEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->x() * this->DevicePixelRatio,
                                   e2->y() * this->DevicePixelRatio,
                                (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                                (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0,
                                0,
                                e2->type() == QEvent::MouseButtonDblClick ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    if(t == QEvent::MouseMove)
    {
      iren->InvokeEvent(vtkCommand::MouseMoveEvent, e2);
    }
    else if(t == QEvent::MouseButtonPress || t == QEvent::MouseButtonDblClick)
    {
      switch(e2->button())
      {
        case Qt::LeftButton:
          iren->InvokeEvent(vtkCommand::LeftButtonPressEvent, e2);
          break;

        case Qt::MidButton:
          iren->InvokeEvent(vtkCommand::MiddleButtonPressEvent, e2);
          break;

        case Qt::RightButton:
          iren->InvokeEvent(vtkCommand::RightButtonPressEvent, e2);
          break;

        default:
          break;
      }
    }
    else if(t == QEvent::MouseButtonRelease)
    {
      switch(e2->button())
      {
        case Qt::LeftButton:
          iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, e2);
          break;

        case Qt::MidButton:
          iren->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, e2);
          break;

        case Qt::RightButton:
          iren->InvokeEvent(vtkCommand::RightButtonReleaseEvent, e2);
          break;

        default:
          break;
      }
    }
    return true;
  }
  if (t == QEvent::TouchBegin ||
      t == QEvent::TouchUpdate ||
      t == QEvent::TouchEnd)
  {
    QTouchEvent* e2 = dynamic_cast<QTouchEvent*>(e);
    foreach (const QTouchEvent::TouchPoint& point, e2->touchPoints())
    {
      if (point.id() >= VTKI_MAX_POINTERS)
      {
        break;
      }
      // give interactor the event information
      iren->SetEventInformationFlipY(point.pos().x() * this->DevicePixelRatio,
                                     point.pos().y() * this->DevicePixelRatio,
                                      (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                                      (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0,
                                      0,0,0, point.id());
    }
    foreach (const QTouchEvent::TouchPoint& point, e2->touchPoints())
    {
      if (point.id() >= VTKI_MAX_POINTERS)
      {
        break;
      }
      iren->SetPointerIndex(point.id());
      if (point.state() & Qt::TouchPointReleased)
      {
        iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
      }
      if (point.state() & Qt::TouchPointPressed)
      {
        iren->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
      if (point.state() & Qt::TouchPointMoved)
      {
        iren->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      }
    }
    e2->accept();
    return true;
  }

  if(t == QEvent::Enter)
  {
    iren->InvokeEvent(vtkCommand::EnterEvent, e);
    return true;
  }

  if(t == QEvent::Leave)
  {
    iren->InvokeEvent(vtkCommand::LeaveEvent, e);
    return true;
  }

  if(t == QEvent::KeyPress || t == QEvent::KeyRelease)
  {
    QKeyEvent* e2 = static_cast<QKeyEvent*>(e);

    // get key and keysym information
    int ascii_key = e2->text().length() ? e2->text().unicode()->toLatin1() : 0;
    const char* keysym = ascii_to_key_sym(ascii_key);
    if(!keysym ||
       e2->modifiers() == Qt::KeypadModifier)
    {
      // get virtual keys
      keysym = qt_key_to_key_sym(static_cast<Qt::Key>(e2->key()),
                                 e2->modifiers());
    }

    if(!keysym)
    {
      keysym = "None";
    }

    // give interactor event information
    iren->SetKeyEventInformation(
      (e2->modifiers() & Qt::ControlModifier),
      (e2->modifiers() & Qt::ShiftModifier),
      ascii_key, e2->count(), keysym);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    if(t == QEvent::KeyPress)
    {
      // invoke vtk event
      iren->InvokeEvent(vtkCommand::KeyPressEvent, e2);

      // invoke char event only for ascii characters
      if(ascii_key)
      {
        iren->InvokeEvent(vtkCommand::CharEvent, e2);
      }
    }
    else
    {
      iren->InvokeEvent(vtkCommand::KeyReleaseEvent, e2);
    }
    return true;
  }

  if(t == QEvent::Wheel)
  {
    QWheelEvent* e2 = static_cast<QWheelEvent*>(e);

    iren->SetEventInformationFlipY(e2->x() * this->DevicePixelRatio,
                                   e2->y() * this->DevicePixelRatio,
                               (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                               (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

#if QT_VERSION >= 0x050000
    this->AccumulatedDelta += e2->angleDelta().y();
#else
    this->AccumulatedDelta += e2->delta();
#endif
    const int threshold = 120;

    // invoke vtk event when accumulated delta passes the threshold
    if(this->AccumulatedDelta >= threshold)
    {
      iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, e2);
      this->AccumulatedDelta = 0;
    }
    else if(this->AccumulatedDelta <= -threshold)
    {
      iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, e2);
      this->AccumulatedDelta = 0;
    }
    return true;
  }

  if(t == QEvent::ContextMenu)
  {
    QContextMenuEvent* e2 = static_cast<QContextMenuEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->x() * this->DevicePixelRatio,
                                   e2->y() * this->DevicePixelRatio,
                               (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                               (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::ContextMenuEvent, e2);

    return true;
  }

  if(t == QEvent::DragEnter)
  {
    QDragEnterEvent* e2 = static_cast<QDragEnterEvent*>(e);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::DragEnterEvent, e2);

    return true;
  }

  if(t == QEvent::DragLeave)
  {
    QDragLeaveEvent* e2 = static_cast<QDragLeaveEvent*>(e);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::DragLeaveEvent, e2);

    return true;
  }

  if(t == QEvent::DragMove)
  {
    QDragMoveEvent* e2 = static_cast<QDragMoveEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->pos().x() * this->DevicePixelRatio,
                                   e2->pos().y() * this->DevicePixelRatio);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::DragMoveEvent, e2);
    return true;
  }

  if(t == QEvent::Drop)
  {
    QDropEvent* e2 = static_cast<QDropEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->pos().x() * this->DevicePixelRatio,
                                   e2->pos().y() * this->DevicePixelRatio);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::DropEvent, e2);
    return true;
  }

  return false;
}

// ***** keysym stuff below  *****

static const char *AsciiToKeySymTable[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, "Tab", 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quoteright",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G",
  "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete",
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const char* ascii_to_key_sym(int i)
{
  if(i >= 0)
  {
    return AsciiToKeySymTable[i];
  }
  return 0;
}

#define QVTK_HANDLE(x,y) \
  case x : \
    ret = y; \
    break;

#define QVTK_HANDLE_KEYPAD(x, y, z) \
  case x : \
    ret = (modifiers & Qt::KeypadModifier) ? (y) : (z); \
    break;

const char* qt_key_to_key_sym(Qt::Key i, Qt::KeyboardModifiers modifiers)
{
  const char* ret = 0;
  switch(i)
  {
    // Cancel
    QVTK_HANDLE(Qt::Key_Backspace, "BackSpace")
      QVTK_HANDLE(Qt::Key_Tab, "Tab")
      QVTK_HANDLE(Qt::Key_Backtab, "Tab")
      QVTK_HANDLE(Qt::Key_Clear, "Clear")
      QVTK_HANDLE(Qt::Key_Return, "Return")
      QVTK_HANDLE(Qt::Key_Enter, "Return")
      QVTK_HANDLE(Qt::Key_Shift, "Shift_L")
      QVTK_HANDLE(Qt::Key_Control, "Control_L")
      QVTK_HANDLE(Qt::Key_Alt, "Alt_L")
      QVTK_HANDLE(Qt::Key_Pause, "Pause")
      QVTK_HANDLE(Qt::Key_CapsLock, "Caps_Lock")
      QVTK_HANDLE(Qt::Key_Escape, "Escape")
      QVTK_HANDLE(Qt::Key_Space, "space")
      QVTK_HANDLE(Qt::Key_PageUp, "Prior")
      QVTK_HANDLE(Qt::Key_PageDown, "Next")
      QVTK_HANDLE(Qt::Key_End, "End")
      QVTK_HANDLE(Qt::Key_Home, "Home")
      QVTK_HANDLE(Qt::Key_Left, "Left")
      QVTK_HANDLE(Qt::Key_Up, "Up")
      QVTK_HANDLE(Qt::Key_Right, "Right")
      QVTK_HANDLE(Qt::Key_Down, "Down")
      QVTK_HANDLE(Qt::Key_Select, "Select")
      QVTK_HANDLE(Qt::Key_Execute, "Execute")
      QVTK_HANDLE(Qt::Key_SysReq, "Snapshot")
      QVTK_HANDLE(Qt::Key_Insert, "Insert")
      QVTK_HANDLE(Qt::Key_Delete, "Delete")
      QVTK_HANDLE(Qt::Key_Help, "Help")
      QVTK_HANDLE_KEYPAD(Qt::Key_0, "KP_0", "0")
      QVTK_HANDLE_KEYPAD(Qt::Key_1, "KP_1", "1")
      QVTK_HANDLE_KEYPAD(Qt::Key_2, "KP_2", "2")
      QVTK_HANDLE_KEYPAD(Qt::Key_3, "KP_3", "3")
      QVTK_HANDLE_KEYPAD(Qt::Key_4, "KP_4", "4")
      QVTK_HANDLE_KEYPAD(Qt::Key_5, "KP_5", "5")
      QVTK_HANDLE_KEYPAD(Qt::Key_6, "KP_6", "6")
      QVTK_HANDLE_KEYPAD(Qt::Key_7, "KP_7", "7")
      QVTK_HANDLE_KEYPAD(Qt::Key_8, "KP_8", "8")
      QVTK_HANDLE_KEYPAD(Qt::Key_9, "KP_9", "9")
      QVTK_HANDLE(Qt::Key_A, "a")
      QVTK_HANDLE(Qt::Key_B, "b")
      QVTK_HANDLE(Qt::Key_C, "c")
      QVTK_HANDLE(Qt::Key_D, "d")
      QVTK_HANDLE(Qt::Key_E, "e")
      QVTK_HANDLE(Qt::Key_F, "f")
      QVTK_HANDLE(Qt::Key_G, "g")
      QVTK_HANDLE(Qt::Key_H, "h")
      QVTK_HANDLE(Qt::Key_I, "i")
      QVTK_HANDLE(Qt::Key_J, "h")
      QVTK_HANDLE(Qt::Key_K, "k")
      QVTK_HANDLE(Qt::Key_L, "l")
      QVTK_HANDLE(Qt::Key_M, "m")
      QVTK_HANDLE(Qt::Key_N, "n")
      QVTK_HANDLE(Qt::Key_O, "o")
      QVTK_HANDLE(Qt::Key_P, "p")
      QVTK_HANDLE(Qt::Key_Q, "q")
      QVTK_HANDLE(Qt::Key_R, "r")
      QVTK_HANDLE(Qt::Key_S, "s")
      QVTK_HANDLE(Qt::Key_T, "t")
      QVTK_HANDLE(Qt::Key_U, "u")
      QVTK_HANDLE(Qt::Key_V, "v")
      QVTK_HANDLE(Qt::Key_W, "w")
      QVTK_HANDLE(Qt::Key_X, "x")
      QVTK_HANDLE(Qt::Key_Y, "y")
      QVTK_HANDLE(Qt::Key_Z, "z")
      QVTK_HANDLE(Qt::Key_Asterisk, "asterisk")
      QVTK_HANDLE(Qt::Key_Plus, "plus")
      QVTK_HANDLE(Qt::Key_Bar, "bar")
      QVTK_HANDLE(Qt::Key_Minus, "minus")
      QVTK_HANDLE(Qt::Key_Period, "period")
      QVTK_HANDLE(Qt::Key_Slash, "slash")
      QVTK_HANDLE(Qt::Key_F1, "F1")
      QVTK_HANDLE(Qt::Key_F2, "F2")
      QVTK_HANDLE(Qt::Key_F3, "F3")
      QVTK_HANDLE(Qt::Key_F4, "F4")
      QVTK_HANDLE(Qt::Key_F5, "F5")
      QVTK_HANDLE(Qt::Key_F6, "F6")
      QVTK_HANDLE(Qt::Key_F7, "F7")
      QVTK_HANDLE(Qt::Key_F8, "F8")
      QVTK_HANDLE(Qt::Key_F9, "F9")
      QVTK_HANDLE(Qt::Key_F10, "F10")
      QVTK_HANDLE(Qt::Key_F11, "F11")
      QVTK_HANDLE(Qt::Key_F12, "F12")
      QVTK_HANDLE(Qt::Key_F13, "F13")
      QVTK_HANDLE(Qt::Key_F14, "F14")
      QVTK_HANDLE(Qt::Key_F15, "F15")
      QVTK_HANDLE(Qt::Key_F16, "F16")
      QVTK_HANDLE(Qt::Key_F17, "F17")
      QVTK_HANDLE(Qt::Key_F18, "F18")
      QVTK_HANDLE(Qt::Key_F19, "F19")
      QVTK_HANDLE(Qt::Key_F20, "F20")
      QVTK_HANDLE(Qt::Key_F21, "F21")
      QVTK_HANDLE(Qt::Key_F22, "F22")
      QVTK_HANDLE(Qt::Key_F23, "F23")
      QVTK_HANDLE(Qt::Key_F24, "F24")
      QVTK_HANDLE(Qt::Key_NumLock, "Num_Lock")
      QVTK_HANDLE(Qt::Key_ScrollLock, "Scroll_Lock")

      default:
    break;
  }
  return ret;
}
