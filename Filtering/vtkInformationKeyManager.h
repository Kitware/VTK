/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationKeyManager - Manages the vtkInformationKey singleton.
// .SECTION Description
// vtkInformationKeyManager should be included in any translation unit
// that will use vtkInformationKey or that implements the singleton
// pattern.  It makes sure that the vtkInformationKey singleton is created
// before and destroyed after it is used.

#ifndef __vtkInformationKeyManager_h
#define __vtkInformationKeyManager_h

#include "vtkSystemIncludes.h"

class VTK_FILTERING_EXPORT vtkInformationKeyManager
{
public:
  vtkInformationKeyManager();
  ~vtkInformationKeyManager();
};

// This instance will show up in any translation unit that uses
// vtkInformationKey or that has a singleton.  It will make sure
// vtkInformationKey is initialized before and destroyed after it is
// used.
static vtkInformationKeyManager vtkInformationKeyManagerInstance;

#endif
