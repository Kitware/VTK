// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "QTestApp.h"

#include <stdio.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

int QTestApp::Error = 0;

QTestApp::QTestApp(int _argc, char* _argv[])
{
  qInstallMessageHandler(QTestApp::messageHandler);

  // CMake generated driver removes argv[0],
  // so let's put a dummy back in
  this->Argv.append("qTestApp");
  for (int i = 0; i < _argc; i++)
  {
    this->Argv.append(_argv[i]);
  }
  for (int j = 0; j < this->Argv.size(); j++)
  {
    this->Argvp.append(this->Argv[j].data());
  }
  this->Argc = this->Argvp.size();
  App = new QApplication(this->Argc, this->Argvp.data());
}

QTestApp::~QTestApp()
{
  delete App;
  qInstallMessageHandler(nullptr);
}

int QTestApp::exec()
{
  if (!QCoreApplication::arguments().contains("--no_exit"))
  {
    QTimer::singleShot(1000, QCoreApplication::instance(), SLOT(quit()));
  }

  int ret = QApplication::exec();
  return Error + ret;
}

void QTestApp::messageHandler(
  QtMsgType type, const QMessageLogContext& context, const QString& message)
{
  Q_UNUSED(context)
  const char* msg = qPrintable(message);
  switch (type)
  {
    case QtDebugMsg:
      fprintf(stderr, "Debug: %s\n", msg);
      break;
    case QtInfoMsg:
      fprintf(stderr, "Info: %s\n", msg);
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
  if (ms > 0)
  {
    QTimer::singleShot(ms, QApplication::instance(), SLOT(quit()));
    QApplication::exec();
  }
}

void QTestApp::simulateEvent(QWidget* w, QEvent* e)
{
  bool status = QApplication::sendEvent(w, e);
  if (!status)
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

void QTestApp::mouseDown(
  QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonPress, pos, screenpos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseUp(
  QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonRelease, pos, screenpos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseMove(
  QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseMove, pos, screenpos, btn, btn, mod);
  simulateEvent(w, &e);
}

void QTestApp::mouseClick(
  QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  mouseDown(w, pos, screenpos, btn, mod, 0);
  mouseUp(w, pos, screenpos, btn, mod, 0);
}
