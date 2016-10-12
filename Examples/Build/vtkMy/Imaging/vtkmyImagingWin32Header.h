/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmyImagingWin32Header.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkmyImagingWin32Header
 * @brief   manage Windows system differences
 *
 * The vtkmyImagingWin32Header captures some system differences between Unix
 * and Windows operating systems.
*/

#ifndef vtkmyImagingWin32Header_h
#define vtkmyImagingWin32Header_h

#include <vtkmyConfigure.h>

#if defined(_WIN32) && !defined(VTKMY_STATIC)
#if defined(vtkmyImaging_EXPORTS)
#define VTK_MY_IMAGING_EXPORT __declspec( dllexport )
#else
#define VTK_MY_IMAGING_EXPORT __declspec( dllimport )
#endif
#else
#define VTK_MY_IMAGING_EXPORT
#endif

#endif
