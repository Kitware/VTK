// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkLegacy_h
#define vtkLegacy_h

//----------------------------------------------------------------------------
//   _  _   ___   _____  ___                          _  _   ___   _____  ___
//  | \| | / _ \ |_   _|| __|                        | \| | / _ \ |_   _|| __|
//  | .  || (_) |  | |  | _|                         | .  || (_) |  | |  | _|
//  |_|\_| \___/   |_|  |___|                        |_|\_| \___/   |_|  |___|
//
// The mechanisms present in this file should no longer be used are are
// are non-functional. Instead, the mechanisms present in `vtkDeprecation.h`
// should be used. The benefits:
//
//   - documentation of *when* the method was removed
//   - support for ignoring warnings if an older VTK is also expected to work
//   - no VTK build will magically take methods away from clients
//----------------------------------------------------------------------------

#include "vtkDeprecation.h"

namespace
{
VTK_DEPRECATED_IN_9_7_0("This header is deprecated, use vtkDeprecation.h instead")
constexpr int vtkLegacyIsDeprecated = 0;
constexpr int vtkLegacyIsReallyDeprecated = vtkLegacyIsDeprecated;
}

/* Compatibility settings.  */
#define VTK_LEGACY_REMOVE 1
#define VTK_LEGACY_SILENT 0
#define VTK_LEGACY_BODY(method, version)
#define VTK_LEGACY_REPLACED_BODY(method, version, replace)

#endif
