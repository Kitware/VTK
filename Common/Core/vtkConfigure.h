/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConfigure.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkConfigure_h
#define vtkConfigure_h

#include "vtkLegacy.h"

#ifndef VTK_LEGACY_REMOVE
#include "vtkBuild.h"
#include "vtkCompiler.h"
#include "vtkDebug.h"
#include "vtkDebugRangeIterators.h"
#include "vtkEndian.h"
#include "vtkFeatures.h"
#include "vtkOptions.h"
#include "vtkPlatform.h"
#include "vtkSMP.h"
#include "vtkThreads.h"

#include "vtkConfigureDeprecated.h"

#include "vtkVersionMacros.h"

/* Legacy macros that are not required as VTK requires a C++11 compiler */
#define VTK_OVERRIDE override
#define VTK_FINAL final
#define VTK_DELETE_FUNCTION = delete
#elif !defined(VTK_LEGACY_SILENT)
#ifdef _MSC_VER
#pragma message(                                                                                   \
  "vtkConfigure.h is deprecated. Please include the relevant header for what you need instead.")
#else
#warning                                                                                           \
  "vtkConfigure.h is deprecated. Please include the relevant header for what you need instead."
#endif
#endif

#endif // vtkConfigure_h

// VTK-HeaderTest-Exclude: vtkConfigure.h
