/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLoggerDisableSignalHandler.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
