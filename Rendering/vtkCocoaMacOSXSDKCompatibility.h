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
// VTK uses types that were introduced with the 10.5 SDK.
// Until VTK's minimum requirement is the 10.5 SDK or later,
// this file allows us to use these new types even with older SDKs
// (where the types are not defined).
// It is safe to include this header multiple times.

#ifdef __OBJC__

  #import <Foundation/Foundation.h>

  #ifndef NSINTEGER_DEFINED
    #ifdef NS_BUILD_32_LIKE_64
      typedef long NSInteger;
      typedef unsigned long NSUInteger;
    #else
      typedef int NSInteger;
      typedef unsigned int NSUInteger;
    #endif
    #define NSIntegerMax    LONG_MAX
    #define NSIntegerMin    LONG_MIN
    #define NSUIntegerMax   ULONG_MAX
    #define NSINTEGER_DEFINED 1
  #endif

#endif

#ifndef CGFLOAT_DEFINED
  typedef float CGFloat;
  #define CGFLOAT_MIN FLT_MIN
  #define CGFLOAT_MAX FLT_MAX
  #define CGFLOAT_IS_DOUBLE 0
  #define CGFLOAT_DEFINED 1
#endif
