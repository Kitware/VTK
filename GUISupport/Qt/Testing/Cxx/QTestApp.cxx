/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QTestApp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "QTestApp.h"

#include <stdio.h>

#include <QTimer>
#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>

int QTestApp::Error = 0;

QTestApp::QTestApp(int _argc, char** _argv)
{
  qInstallMsgHandler(QTestApp::messageHandler);

  // CMake generated driver removes argv[0],
  // so let's put a dummy back in
  this->Argv.append("qTestApp");
  for(int i=0; i<_argc; i++)
    {
    this->Argv.append(_argv[i]);
    }
  for(int j=0; j<this->Argv.size(); j++)
    {
    this->Argvp.append(this->Argv[j].data());
    }
  this->Argc = this->Argvp.size();
  App = new QApplication(this->Argc, this->Argvp.data());
}

QTestApp::~QTestApp()
{
  delete App;
  qInstallMsgHandler(0);
}

int QTestApp::exec()
{
  if(!QCoreApplication::arguments().contains("--no_exit"))
    {
    QTimer::singleShot(1000, QCoreApplication::instance(), SLOT(quit()));
    }

  int ret = QApplication::exec();
  return Error + ret;
}

void QTestApp::messageHandler(QtMsgType type, const char *msg)
{
  switch(type)
  {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg);
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s\n", msg);
    Error++;
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg);
    Error++;
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s\n", msg);
    abort();
  }
}

void QTestApp::delay(int ms)
{
  if(ms > 0)
  {
    QTimer::singleShot(ms, QApplication::instance(), SLOT(quit()));
    QApplication::exec();
  }
}

void QTestApp::simulateEvent(QWidget* w, QEvent* e)
{
  bool status = QApplication::sendEvent(w, e);
  if(!status)
  {
    qWarning("event not handled\n");
  }
  QApplication::processEvents();
}

void QTestApp::keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QKeyEvent e(QEvent::KeyRelease, key, mod);
  simulateEvent(w, &e);
}

void QTestApp::keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QKeyEvent e(QEvent::KeyPress, key, mod);
  simulateEvent(w, &e);
}

void QTestApp::keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  keyDown(w, key, mod, 0);
  keyUp(w, key, mod, 0);
}

void QTestApp::mouseDown(QWidget* w, QPoint pos, Qt::MouseButton btn,
                        Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonPress, pos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseUp(QWidget* w, QPoint pos, Qt::MouseButton btn,
                      Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonRelease, pos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseMove(QWidget* w, QPoint pos, Qt::MouseButton btn,
                        Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseMove, pos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseClick(QWidget* w, QPoint pos, Qt::MouseButton btn,
                         Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  mouseDown(w, pos, btn, mod, 0);
  mouseUp(w, pos, btn, mod, 0);
}


