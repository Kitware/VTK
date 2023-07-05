// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCocoaMacOSXSDKCompatibility
 * @brief   Compatibility header
 *
 * VTK requires the Mac OS X 10.7 SDK or later.
 * However, this file is meant to allow us to use features from newer
 * SDKs by adding workarounds to still support the minimum SDK.
 * It is safe to include this header multiple times.
 */

#include <AvailabilityMacros.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
#error VTK requires the Mac OS X 10.7 SDK or later
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
#error VTK requires a deployment target of Mac OS X 10.7 or later
#endif

// Stop AssertMacros.h from defining its macros without underscore prefixes,
// which pollute the global namespace and cause us build issues.
// This is default as of the macOS 10.13 SDK, but needed for older SDKs.
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101300
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif

#if (MAC_OS_X_VERSION_MAX_ALLOWED < 101200) && !defined(VTK_DONT_MAP_10_12_ENUMS)
// The 10.12 SDK made a bunch of enum names more logical, map old names to new names to continue
// supporting old SDKs.
#define NSWindowStyleMask NSUInteger
#define NSWindowStyleMaskBorderless NSBorderlessWindowMask
#define NSWindowStyleMaskTitled NSTitledWindowMask
#define NSWindowStyleMaskClosable NSClosableWindowMask
#define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
#define NSWindowStyleMaskResizable NSResizableWindowMask

#define NSEventModifierFlagShift NSShiftKeyMask
#define NSEventModifierFlagControl NSControlKeyMask
#define NSEventModifierFlagOption NSAlternateKeyMask
#define NSEventModifierFlagCommand NSCommandKeyMask

#define NSEventTypeKeyDown NSKeyDown
#define NSEventTypeKeyUp NSKeyUp
#define NSEventTypeApplicationDefined NSApplicationDefined
#define NSEventTypeFlagsChanged NSFlagsChanged

#define NSEventMaskAny NSAnyEventMask
#endif

// Create handy #defines that indicate the Objective-C memory management model.
// Manual Retain Release, Automatic Reference Counting, or Garbage Collection.
#if defined(__OBJC_GC__)
#define VTK_OBJC_IS_MRR 0
#define VTK_OBJC_IS_ARC 0
#define VTK_OBJC_IS_GC 1
#elif __has_feature(objc_arc)
#define VTK_OBJC_IS_MRR 0
#define VTK_OBJC_IS_ARC 1
#define VTK_OBJC_IS_GC 0
#else
#define VTK_OBJC_IS_MRR 1
#define VTK_OBJC_IS_ARC 0
#define VTK_OBJC_IS_GC 0
#endif

#if __has_feature(objc_arc)
#error VTK does not yet support ARC memory management
#endif

// VTK-HeaderTest-Exclude: vtkCocoaMacOSXSDKCompatibility.h
