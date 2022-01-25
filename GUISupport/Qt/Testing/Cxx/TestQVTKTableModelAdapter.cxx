/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTableModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "QVTKTableModelAdapterTestClass.h"

#include <QCoreApplication>
#include <QTimer>

int TestQVTKTableModelAdapter(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QVTKTableModelAdapterTestClass test(&app);
  QTimer::singleShot(100, &test, SLOT(runTests()));
  return QCoreApplication::exec();
}
