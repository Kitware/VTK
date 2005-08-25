/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaterialLibraryConfig.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkMaterialLibraryConfig_h
#define __vtkMaterialLibraryConfig_h

#include "MaterialLibraryCMakeConfig.h"

//
// BEGIN Toolkit (ITK,VTK, etc) specific
// 
#ifdef vtkMaterialLibrary_EXPORTS
  #define MATERIAL_LIBRARY_EXPORT_SYMBOLS
#endif
//
// END toolkit (ITK, VTK, etc) specific
//

#ifdef MATERIAL_LIBRARY_DLL 
  #ifdef MATERIAL_LIBRARY_EXPORT_SYMBOLS
    #define VTK_MATERIAL_LIBRARY_EXPORT __declspec(dllexport)
  #else
    #define VTK_MATERIAL_LIBRARY_EXPORT __declspec(dllimport)
  #endif
#else
  #define VTK_MATERIAL_LIBRARY_EXPORT 
#endif

#endif
