/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWin32Header.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#ifndef __QVTKWin32Header_h
#define __QVTKWin32Header_h

#include "vtkConfigure.h"

#if defined(WIN32) && defined(VTK_BUILD_SHARED_LIBS)
#if defined(QVTK_EXPORTS) || defined(QVTKWidgetPlugin_EXPORTS)
#define QVTK_EXPORT __declspec( dllexport )
#else
#define QVTK_EXPORT __declspec( dllimport ) 
#endif
#else
#define QVTK_EXPORT
#endif

#endif /*__QVTKWin32Header_h*/
