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
static const char* qt_key_to_key_sym(Qt::Key);

QVTKInteractorAdapter::QVTKInteractorAdapter(QObject* parentObject)
  : QObject(parentObject)
{
}

QVTKInteractorAdapter::~QVTKInteractorAdapter()
{
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
    iren->SetSize(size.width(), size.height());
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
    iren->SetEventInformationFlipY(e2->x(), e2->y(),
                                (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                                (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0,
                                0,
                                e2->type() == QEvent::MouseButtonDblClick ? 1 : 0);

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
    if(!keysym)
      {
      // get virtual keys
      keysym = qt_key_to_key_sym(static_cast<Qt::Key>(e2->key()));
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

    iren->SetEventInformationFlipY(e2->x(), e2->y(),
                               (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                               (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);

    // invoke vtk event
    // if delta is positive, it is a forward wheel event
    if(e2->delta() > 0)
      {
      iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, e2);
      }
    else
      {
      iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, e2);
      }
    return true;
    }

  if(t == QEvent::ContextMenu)
    {
    QContextMenuEvent* e2 = static_cast<QContextMenuEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->x(), e2->y(),
                               (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
                               (e2->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);

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
    iren->SetEventInformationFlipY(e2->pos().x(), e2->pos().y());

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QVTKInteractor::DragMoveEvent, e2);
    return true;
    }

  if(t == QEvent::Drop)
    {
    QDropEvent* e2 = static_cast<QDropEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(e2->pos().x(), e2->pos().y());

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

const char* qt_key_to_key_sym(Qt::Key i)
{
  const char* ret = 0;
  switch(i)
    {
    // Cancel
    QVTK_HANDLE(Qt::Key_Backspace, "BackSpace")
      QVTK_HANDLE(Qt::Key_Tab, "Tab")
      QVTK_HANDLE(Qt::Key_Backtab, "Tab")
      //QVTK_HANDLE(Qt::Key_Clear, "Clear")
      QVTK_HANDLE(Qt::Key_Return, "Return")
      QVTK_HANDLE(Qt::Key_Enter, "Return")
      QVTK_HANDLE(Qt::Key_Shift, "Shift_L")
      QVTK_HANDLE(Qt::Key_Control, "Control_L")
      QVTK_HANDLE(Qt::Key_Alt, "Alt_L")
      QVTK_HANDLE(Qt::Key_Pause, "Pause")
      QVTK_HANDLE(Qt::Key_CapsLock, "Caps_Lock")
      QVTK_HANDLE(Qt::Key_Escape, "Escape")
      QVTK_HANDLE(Qt::Key_Space, "space")
      //QVTK_HANDLE(Qt::Key_Prior, "Prior")
      //QVTK_HANDLE(Qt::Key_Next, "Next")
      QVTK_HANDLE(Qt::Key_End, "End")
      QVTK_HANDLE(Qt::Key_Home, "Home")
      QVTK_HANDLE(Qt::Key_Left, "Left")
      QVTK_HANDLE(Qt::Key_Up, "Up")
      QVTK_HANDLE(Qt::Key_Right, "Right")
      QVTK_HANDLE(Qt::Key_Down, "Down")

      // Select
      // Execute
      QVTK_HANDLE(Qt::Key_SysReq, "Snapshot")
      QVTK_HANDLE(Qt::Key_Insert, "Insert")
      QVTK_HANDLE(Qt::Key_Delete, "Delete")
      QVTK_HANDLE(Qt::Key_Help, "Help")
      QVTK_HANDLE(Qt::Key_0, "0")
      QVTK_HANDLE(Qt::Key_1, "1")
      QVTK_HANDLE(Qt::Key_2, "2")
      QVTK_HANDLE(Qt::Key_3, "3")
      QVTK_HANDLE(Qt::Key_4, "4")
      QVTK_HANDLE(Qt::Key_5, "5")
      QVTK_HANDLE(Qt::Key_6, "6")
      QVTK_HANDLE(Qt::Key_7, "7")
      QVTK_HANDLE(Qt::Key_8, "8")
      QVTK_HANDLE(Qt::Key_9, "9")
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
      // KP_0 - KP_9
      QVTK_HANDLE(Qt::Key_Asterisk, "asterisk")
      QVTK_HANDLE(Qt::Key_Plus, "plus")
      // bar
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
