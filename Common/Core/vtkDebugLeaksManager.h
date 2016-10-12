/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaksManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDebugLeaksManager
 * @brief   Manages the vtkDebugLeaks singleton.
 *
 * vtkDebugLeaksManager should be included in any translation unit
 * that will use vtkDebugLeaks or that implements the singleton
 * pattern.  It makes sure that the vtkDebugLeaks singleton is created
 * before and destroyed after all other singletons in VTK.
*/

#ifndef vtkDebugLeaksManager_h
#define vtkDebugLeaksManager_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

class VTKCOMMONCORE_EXPORT vtkDebugLeaksManager
{
public:
  vtkDebugLeaksManager();
  ~vtkDebugLeaksManager();
private:
  vtkDebugLeaksManager(const vtkDebugLeaksManager&);
  vtkDebugLeaksManager& operator=(
    const vtkDebugLeaksManager&);
};

// This instance will show up in any translation unit that uses
// vtkDebugLeaks or that has a singleton.  It will make sure
// vtkDebugLeaks is initialized before it is used and is the last
// static object destroyed.
static vtkDebugLeaksManager vtkDebugLeaksManagerInstance;

#endif
// VTK-HeaderTest-Exclude: vtkDebugLeaksManager.h
