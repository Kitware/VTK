// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef QTestApp_h
#define QTestApp_h

#include <QApplication>
#include <QByteArray>
#include <QVector>

class QTestApp
{
public:
  QTestApp(int _argc, char** _argv);
  ~QTestApp();

  static int exec();

  static void messageHandler(
    QtMsgType type, const QMessageLogContext& context, const QString& message);

  static void delay(int ms);

  static void simulateEvent(QWidget* w, QEvent* e);

  static void keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void mouseDown(QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn,
    Qt::KeyboardModifiers mod, int ms);

  static void mouseUp(QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn,
    Qt::KeyboardModifiers mod, int ms);

  static void mouseMove(QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn,
    Qt::KeyboardModifiers mod, int ms);

  static void mouseClick(QWidget* w, QPoint pos, QPoint screenpos, Qt::MouseButton btn,
    Qt::KeyboardModifiers mod, int ms);

private:
  QApplication* App;
  static int Error;
  QList<QByteArray> Argv;
  QVector<char*> Argvp;
  int Argc;
};

#endif
