// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// Tests vtkQtInitialization.
// Thanks to Tim Shead from Sandia National Laboratories for writing this test.

#include "vtkQtInitialization.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <QCoreApplication>

#include <iostream>

int TestQtInitialization(int, char*[])
{
  int error_count = 0;

  // Because we share the same process with other tests, verify that
  // an instance of QCoreApplication hasn't already been created.  This
  // ensures that we don't introduce false-positives in case some other test
  // has an instance of QCoreApplication floating-around ...

  if (QCoreApplication::instance())
  {
    std::cerr << "Internal test error ... QCoreApplication already exists" << std::endl;
    ++error_count;
  }

  vtkSmartPointer<vtkQtInitialization> initialization = vtkSmartPointer<vtkQtInitialization>::New();

  if (!QCoreApplication::instance())
  {
    std::cerr << "QCoreApplication not initialized" << std::endl;
    ++error_count;
  }

  return error_count;
}
