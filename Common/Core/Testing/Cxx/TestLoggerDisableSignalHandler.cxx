// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test disabling vtkLogger's printing of a stack trace when a signal is handled.

#include "vtkLogger.h"

#include <cstdlib>

int main(int, char*[])
{
  // When set to false, no stack trace should be emitted when vtkLogger
  // catches the SIGABRT signal below.
  vtkLogger::EnableUnsafeSignalHandler = false;
  vtkLogger::Init();

  abort();

  return EXIT_SUCCESS;
}
