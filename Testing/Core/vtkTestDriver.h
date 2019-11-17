/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestDriver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This header is included by all the C++ test drivers in VTK.
#ifndef vtkTestDriver_h
#define vtkTestDriver_h

#include "vtkFloatingPointExceptions.h"
#include <exception> // for std::exception

#include <clocale> // C setlocale()
#include <locale>  // C++ locale

#include <vtksys/SystemInformation.hxx> // for stacktrace

#include <vtkLogger.h> // for logging

#include "vtkWindowsTestUtilities.h" // for windows stack trace

#endif
// VTK-HeaderTest-Exclude: vtkTestDriver.h
