// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
