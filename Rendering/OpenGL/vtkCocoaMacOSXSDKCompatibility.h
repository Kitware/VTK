/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCocoaMacOSXSDKCompatibility.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCocoaMacOSXSDKCompatibility - Compatibility header
// .SECTION Description
// VTK requires the Mac OS X 10.5 SDK or later.
// However, this file is meant to allow us to use features from newer
// SDKs by adding workarounds to still support the minimum SDK.
// Currently, there is no such need, but there was in the past, and
// keeping this file allows for the possibility again in the future.
// It is safe to include this header multiple times.

#include <AvailabilityMacros.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1050
  #error VTK requires the Mac OS X 10.5 SDK or later
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
  #error VTK requires a deployment target of Mac OS X 10.5 or later
#endif
// VTK-HeaderTest-Exclude: vtkCocoaMacOSXSDKCompatibility.h
