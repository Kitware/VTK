/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeExport.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBridgeExport
 * @brief   manage Windows system differences
 *
 * The vtkBridgeExport captures some system differences between Unix and
 * Windows operating systems.
*/

#ifndef vtkBridgeExport_h
#define vtkBridgeExport_h
#include "vtkTestingGenericBridgeModule.h"
#include "vtkSystemIncludes.h"

#if 1
# define VTK_BRIDGE_EXPORT
#else

#if defined(_WIN32) && defined(VTK_BUILD_SHARED_LIBS)

 #if defined(vtkBridge_EXPORTS)
  #define VTK_BRIDGE_EXPORT __declspec( dllexport )
 #else
  #define VTK_BRIDGE_EXPORT __declspec( dllimport )
 #endif
#else
 #define VTK_BRIDGE_EXPORT
#endif

#endif //#if 1

#endif

// VTK-HeaderTest-Exclude: vtkBridgeExport.h
