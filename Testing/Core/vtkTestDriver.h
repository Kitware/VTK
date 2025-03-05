// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This header is included by all the C++ test drivers in VTK.
#ifndef vtkTestDriver_h
#define vtkTestDriver_h

#include "vtkFloatingPointExceptions.h"
#include <exception> // for std::exception

#include <clocale> // C setlocale()
#include <locale>  // C++ locale

#include <vtksys/SystemInformation.hxx> // for stacktrace

#include <vtkLogger.h> // for logging

#include "vtkEmscriptenTestUtilities.h" // for wasm I/O and exit helper
#include "vtkWindowsTestUtilities.h"    // for windows stack trace

#endif
// VTK-HeaderTest-Exclude: vtkTestDriver.h
