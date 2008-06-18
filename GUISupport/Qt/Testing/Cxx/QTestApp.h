/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QTestApp.h

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

#ifndef QTestApp_h
#define QTestApp_h

#include <QApplication>
#include <QVector>
#include <QByteArray>

class QTestApp
{
public:
  QTestApp(int _argc, char** _argv);
  ~QTestApp();

  static int exec();

  static void messageHandler(QtMsgType type, const char *msg);

  static void delay(int ms);

  static void simulateEvent(QWidget* w, QEvent* e);

  static void keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void mouseDown(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms);
  
  static void mouseUp(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                      Qt::KeyboardModifiers mod, int ms);
  
  static void mouseMove(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms);

  static void mouseClick(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                         Qt::KeyboardModifiers mod, int ms);

private:
  QApplication* App;
  static int Error;
  QList<QByteArray> Argv;
  QVector<char*> Argvp;
  int Argc;
};

#endif

