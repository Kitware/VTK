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
# include <jpeglib.h>
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
