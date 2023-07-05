// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBridgeExport
 * @brief   manage Windows system differences
 *
 * The vtkBridgeExport captures some system differences between Unix and
 * Windows operating systems.
 */

#ifndef vtkBridgeExport_h
#define vtkBridgeExport_h
#include "vtkBuild.h"
#include "vtkSystemIncludes.h"
#include "vtkTestingGenericBridgeModule.h"

#if 1
#define VTK_BRIDGE_EXPORT
#else

#if defined(_WIN32) && defined(VTK_BUILD_SHARED_LIBS)

#if defined(vtkBridge_EXPORTS)
#define VTK_BRIDGE_EXPORT __declspec(dllexport)
#else
#define VTK_BRIDGE_EXPORT __declspec(dllimport)
#endif
#else
#define VTK_BRIDGE_EXPORT
#endif

#endif //#if 1

#endif

// VTK-HeaderTest-Exclude: vtkBridgeExport.h
