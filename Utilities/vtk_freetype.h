/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk_freetype.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtk_freetype_h
#define __vtk_freetype_h

/* Use the freetype library configured for VTK.  */
#include "vtkToolkits.h"
#ifdef VTK_USE_SYSTEM_FREETYPE
#ifndef FTC_Manager_LookupFace
#define FTC_Manager_LookupFace FTC_Manager_Lookup_Face
#endif //FTC_Manager_LookupFace
#ifndef FTC_Manager_LookupSize
#define FTC_Manager_LookupSize FTC_Manager_Lookup_Size
#endif //FTC_Manager_LookupSize
# include <ft2build.h>
#else
# include <vtkfreetype/include/ft2build.h>
#  if defined(VTKFREETYPE)
#    include "vtkFreeTypeConfig.h"
#  endif
#endif

#endif
