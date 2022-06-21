/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCConfigure.h.in

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMFCConfigure_h
#define vtkMFCConfigure_h

#include "vtkLegacy.h"

#ifndef VTK_LEGACY_SILENT // VTK_DEPRECATED_IN_9_2_0
#ifdef _MSC_VER
#pragma message("vtkMFCConfigure.h is deprecated; there is no need for the header.")
#else
#warning "vtkMFCConfigure.h is deprecated; there is no need for the header."
#endif
#endif

#endif

// VTK-HeaderTest-Exclude: vtkMFCConfigure.h
