// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file    vtkOpenXRPlatform.h
 * @brief   Defines the window system being used and graphics extensions.
 *
 * While the window system (XR_USE_PLATFORM_WIN32/XR_USE_PLATFORM_XLIB) is
 * automatically defined, one must define XR_USE_GRAPHICS_API_OPENGL or/and
 * XR_USE_GRAPHICS_API_D3D11 before including this header, in order to
 * define the required types for the graphics API being used.
 */

#ifndef vtkOpenXRPlatform_h
#define vtkOpenXRPlatform_h

// Needed for WIN32 and VTK_USE_X
#include "vtkRenderingOpenGLConfigure.h"
#include "vtk_glew.h"

#if defined(VTK_USE_X)
// X11 defines globally some names that conflict with things in these classes
//     X11/Xutil.h contains "#define AllValues 0x000F"
//     X11/Xlib.h contains "#define Status int"
#include "vtkGenericDataArray.h"
#include <vtksys/Status.hxx>
#endif

#ifdef _WIN32
#define XR_USE_PLATFORM_WIN32
#include "GL/gl.h"
#include "Unknwn.h"
#include "vtkWindows.h"
#endif

#ifdef VTK_USE_X
#define XR_USE_PLATFORM_XLIB
// Required headers for the XrGraphicsBindingOpenGLXlibKHR struct
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

#include <XrGraphicsExtensions.h>

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRPlatform.h
