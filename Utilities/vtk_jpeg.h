/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk_jpeg.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtk_jpeg_h
#define __vtk_jpeg_h

/* Use the jpeg library configured for VTK.  */
#include "vtkToolkits.h"
#ifdef VTK_USE_SYSTEM_JPEG
  /* Work-around for a conflict between windows.h and jpeglib.h in cygwin.
     If ADDRESS_TAG_BIT is defined then BaseTsd.h has been included and
     INT32 has been defined with a typedef, so we must define XMD_H to
     prevent the jpeg header from defining it again.  */
# if defined(__CYGWIN__) && defined(ADDRESS_TAG_BIT) && !defined(XMD_H)
#  define XMD_H
#  define VTK_JPEG_XMD_H
# endif
# include <jpeglib.h>
# if defined(VTK_JPEG_XMD_H)
#  undef VTK_JPEG_XMD_H
#  undef XMD_H
# endif
#else
# include <vtkjpeg/jpeglib.h>
#endif

#endif

/* Include the jerror header if VTK_JPEG_INCLUDE_JERROR is defined.  */
#if defined(VTK_JPEG_INCLUDE_JERROR) && !defined(VTK_JPEG_JERROR_INCLUDED)
# define VTK_JPEG_JERROR_INCLUDED
# ifdef VTK_USE_SYSTEM_JPEG
#  include <jerror.h>
# else
#  include <vtkjpeg/jerror.h>
# endif
#endif
